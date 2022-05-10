/* *******************************************************************
   Modbus TCP/UDP functions
   Based on https://github.com/budulinek/arduino-modbus-rtu-tcp-gateway

   recvUdp
   - receives Modbus UDP (or Modbus RTU over UDP) messages
   - calls checkRequest
   - stores requests in queue or replies with error

   recvTcp
   - receives Modbus TCP (or Modbus RTU over TCP) messages
   - calls checkRequest
   - stores requests in queue or replies with error

   processRequests
   - inserts scan request into queue
   - optimizes queue

   checkRequest
   - checks Modbus TCP/UDP requests (correct MBAP header, CRC in case of Modbus RTU over TCP/UDP)
   - checks availability of queue

   deleteRequest
   - deletes requests from queue

   getSlaveResponding, setSlaveResponding
   - read from and write to bool array

   ***************************************************************** */

#ifdef IPMODBUS
#include "ipmodbus.h" 
#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <utility/w5100.h>


byte maxSockNum = MAX_SOCK_NUM;
EthernetUDP Udp;
EthernetServer modbusServer(502);

// each request is stored in 3 queues (all queues are written to, read and deleted in sync)
CircularBuffer<header, reqQueueCount> queueHeaders;            // queue of requests' headers and metadata (MBAP transaction ID, MBAP unit ID, PDU length, remIP, remPort, TCP client)
CircularBuffer<byte, reqQueueSize> queuePDUs;           // queue of PDU data (function code, data)
CircularBuffer<byte, reqQueueCount> queueRetries;              // queue of retry counters


enum state serialState;
unsigned int charTimeout;
unsigned int frameDelay;

// bool array for storing Modbus RTU status (responging or not responding). Array index corresponds to slave address.
uint8_t slavesResponding[(maxSlaves + 1 + 7) / 8];
uint8_t  masks[8] = {1, 2, 4, 8, 16, 32, 64, 128};

const byte scanCommand[] = {0x03, 0x00, 0x00, 0x00, 0x01};  // Command sent during Modbus RTU Scan. Slave is detected if any response (even error) is received.


MicroTimer rxDelay;
MicroTimer rxTimeout;
MicroTimer txDelay;


Timer requestTimeout;
uint16_t crc;
byte scanCounter = 0;

/****** RUN TIME AND DATA COUNTERS ******/

// store uptime seconds (includes seconds counted before millis() overflow)
unsigned long seconds;
// store last millis() so that we can detect millis() overflow
unsigned long last_milliseconds = 0;
// store seconds passed until the moment of the overflow so that we can add them to "seconds" on the next call
unsigned long remaining_seconds = 0;
// Data counters (we only use unsigned long in ENABLE_EXTRA_DIAG, to save flash memory)
#ifdef ENABLE_EXTRA_DIAG
unsigned long serialTxCount = 0;
unsigned long serialRxCount = 0;
unsigned long ethTxCount = 0;
unsigned long ethRxCount = 0;
#else
unsigned int serialTxCount = 0;
unsigned int serialRxCount = 0;
unsigned int ethTxCount = 0;
unsigned int ethRxCount = 0;
#endif /* ENABLE_EXTRA_DIAG */

int rxNdx = 0;
int txNdx = 0;
bool rxErr = false;


void recvUdp()
{
  unsigned int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    ethRxCount += packetSize;
    byte udpInBuffer[modbusSize + 4];          // Modbus TCP frame is 4 bytes longer than Modbus RTU frame
    // Modbus TCP/UDP frame: [0][1] transaction ID, [2][3] protocol ID, [4][5] length and [6] unit ID (address).....
    // Modbus RTU frame: [0] address.....
    Udp.read(udpInBuffer, sizeof(udpInBuffer));
    Udp.flush();

    byte errorCode = checkRequest(udpInBuffer, packetSize);
    byte pduStart;        // first byte of Protocol Data Unit (i.e. Function code)
    if (enableRtuOverTcp) pduStart = 1;   // In Modbus RTU, Function code is second byte (after address)
    else pduStart = 7;            // In Modbus TCP/UDP, Function code is 8th byte (after address)
    if (errorCode == 0) {
      // Store in request queue: 2 bytes MBAP Transaction ID (ignored in Modbus RTU over TCP); MBAP Unit ID (address); PDUlen (func + data);remote IP; remote port; TCP client Number (socket) - 0xFF for UDP
      queueHeaders.push(header {{udpInBuffer[0], udpInBuffer[1]}, udpInBuffer[pduStart - 1], (byte)(packetSize - pduStart), Udp.remoteIP(), Udp.remotePort(), UDP_REQUEST});
      queueRetries.push(0);
      for (byte i = 0; i < (byte)(packetSize - pduStart); i++) {
        queuePDUs.push(udpInBuffer[i + pduStart]);
      }
    } else if (errorCode != 0xFF) {
      // send back message with error code
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      if (!enableRtuOverTcp) {
        Udp.write(udpInBuffer, 5);
        Udp.write(0x03);
      }
      Udp.write(udpInBuffer[pduStart - 1]);   // address
      Udp.write(udpInBuffer[pduStart] + 0x80);       // function + 0x80
      Udp.write(errorCode);
      if (enableRtuOverTcp) {
        crc = 0xFFFF;
        calculateCRC(udpInBuffer[pduStart - 1]);
        calculateCRC(udpInBuffer[pduStart] + 0x80);
        calculateCRC(errorCode);
        Udp.write(lowByte(crc));        // send CRC, low byte first
        Udp.write(highByte(crc));
      }
      Udp.endPacket();
      ethTxCount += 5;
      if (!enableRtuOverTcp) ethTxCount += 4;
    }
  }
}


void recvTcp()
{
  EthernetClient client = modbusServer.available();
  if (client) {
    unsigned int packetSize = client.available();
    ethRxCount += packetSize;
    byte tcpInBuffer[modbusSize + 4];          // Modbus TCP frame is 4 bytes longer than Modbus RTU frame
    // Modbus TCP/UDP frame: [0][1] transaction ID, [2][3] protocol ID, [4][5] length and [6] unit ID (address).....
    // Modbus RTU frame: [0] address.....
    client.read(tcpInBuffer, sizeof(tcpInBuffer));
    client.flush();
    byte errorCode = checkRequest(tcpInBuffer, packetSize);
    byte pduStart;        // first byte of Protocol Data Unit (i.e. Function code)
    if (enableRtuOverTcp) pduStart = 1;   // In Modbus RTU, Function code is second byte (after address)
    else pduStart = 7;            // In Modbus TCP/UDP, Function code is 8th byte (after address)
    //debugSerial<<"TCP modbus received packet. Code="<<errorCode<<" len="<<ethRxCount<<endl;
    if (errorCode == 0) {
      // Store in request queue: 2 bytes MBAP Transaction ID (ignored in Modbus RTU over TCP); MBAP Unit ID (address); PDUlen (func + data);remote IP; remote port; TCP client Number (socket) - 0xFF for UDP
      queueHeaders.push(header {{tcpInBuffer[0], tcpInBuffer[1]}, tcpInBuffer[pduStart - 1], (byte)(packetSize - pduStart), {}, 0, client.getSocketNumber()});
      queueRetries.push(0);
      for (byte i = 0; i < packetSize - pduStart; i++) {
        queuePDUs.push(tcpInBuffer[i + pduStart]);
      }
    } else if (errorCode != 0xFF) {
      // send back message with error code
      if (!enableRtuOverTcp) {
        client.write(tcpInBuffer, 5);
        client.write(0x03);
      }
      client.write(tcpInBuffer[pduStart - 1]);   // address
      client.write(tcpInBuffer[pduStart] + 0x80);       // function + 0x80
      client.write(errorCode);
      if (enableRtuOverTcp) {
        crc = 0xFFFF;
        calculateCRC(tcpInBuffer[pduStart - 1]);
        calculateCRC(tcpInBuffer[pduStart] + 0x80);
        calculateCRC(errorCode);
        client.write(lowByte(crc));        // send CRC, low byte first
        client.write(highByte(crc));
      }
      ethTxCount += 5;
      if (!enableRtuOverTcp) ethTxCount += 4;
    }
  }
}

void processRequests()
{
  // Insert scan request into queue
  if (scanCounter != 0 && queueHeaders.available() > 1 && queuePDUs.available() > 1) {
    // Store scan request in request queue
    queueHeaders.push(header {{0x00, 0x00}, scanCounter, sizeof(scanCommand), {}, 0, SCAN_REQUEST});
    queueRetries.push(serialRetry - 1);     // scan requests are only sent once, so set "queueRetries" to one attempt below limit
    for (byte i = 0; i < sizeof(scanCommand); i++) {
      queuePDUs.push(scanCommand[i]);
    }
    scanCounter++;
    if (scanCounter == maxSlaves + 1) scanCounter = 0;
  }

  // Optimize queue (prioritize requests from responding slaves) and trigger sending via serial
  if (!modbusBusy && (serialState == MBIDLE)) {               // send new data over serial only if we are not waiting for response
    if (!queueHeaders.isEmpty()) {
      boolean queueHasRespondingSlaves;               // true if  queue holds at least one request to responding slaves
      for (byte i = 0; i < queueHeaders.size(); i++) {
        if (getSlaveResponding(queueHeaders[i].uid) == true) {
          queueHasRespondingSlaves = true;
          break;
        } else {
          queueHasRespondingSlaves = false;
        }
      }
      while (queueHasRespondingSlaves == true && getSlaveResponding(queueHeaders.first().uid) == false) {
        // move requests to non responding slaves to the tail of the queue
        for (byte i = 0; i < queueHeaders.first().PDUlen; i++) {
          queuePDUs.push(queuePDUs.shift());
        }
        queueRetries.push(queueRetries.shift());
        queueHeaders.push(queueHeaders.shift());
      }
      serialState = SENDING;                   // trigger sendSerial()
      modbusBusy = true;
    }
  }
}

byte checkRequest(byte buffer[], unsigned int bufferSize) {
  byte address;
  if (enableRtuOverTcp) address = buffer[0];
  else address = buffer[6];

  if (enableRtuOverTcp) {   // check CRC for Modbus RTU over TCP/UDP
    if (checkCRC(buffer, bufferSize) == false) {
      return 0xFF;                         // reject: do nothing and return no error code
    }
  } else {                  // check MBAP header structure for Modbus TCP/UDP
    if (buffer[2] != 0x00 || buffer[3] != 0x00 || buffer[4] != 0x00 || buffer[5] != bufferSize - 6) {
      return 0xFF;                         // reject: do nothing and return no error code
    }
  }
  if (queueHeaders.isEmpty() == false && getSlaveResponding(address) == false) {                       // allow only one request to non responding slaves
    for (byte j = queueHeaders.size(); j > 0 ; j--) {     // start searching from tail because requests to non-responsive slaves are usually towards the tail of the queue
      if (queueHeaders[j - 1].uid == address) {
        return 0x0B;                   // return modbus error 11 (Gateway Target Device Failed to Respond) - usually means that target device (address) is not present
      }
    }
  }
  // check if we have space in request queue
  if (queueHeaders.available() < 1 || (enableRtuOverTcp && queuePDUs.available() < bufferSize - 1) || (!enableRtuOverTcp && queuePDUs.available() < bufferSize - 7)) {
    return 0x06;                       // return modbus error 6 (Slave Device Busy) - try again later
  }
  // al checkes passed OK, we can store the incoming data in request queue
  return 0;
}

void deleteRequest()        // delete request from queue
{
  for (byte i = 0; i < queueHeaders.first().PDUlen; i++) {
    queuePDUs.shift();
  }
  queueHeaders.shift();
  queueRetries.shift();
}


bool getSlaveResponding(const uint8_t index)
{
  if (index >= maxSlaves) return false;     // error
  return (slavesResponding[index / 8] & masks[index & 7]) > 0;
}


void setSlaveResponding(const uint8_t index, const bool value)
{
  if (index >= maxSlaves) return;     // error
  if (value == 0) slavesResponding[index / 8] &= ~masks[index & 7];
  else slavesResponding[index / 8] |= masks[index & 7];
}

/* *******************************************************************
   Modbus RTU functions

   sendSerial
   - sends Modbus RTU requests to HW serial port (RS485 interface)

   recvSerial
   - receives Modbus RTU replies
   - adjusts headers and forward messages as Modbus TCP/UDP or Modbus RTU over TCP/UDP
   - sends Modbus TCP/UDP error messages in case Modbus RTU response timeouts

   checkCRC
   - checks an array and returns true if CRC is OK

   calculateCRC

   ***************************************************************** */



void sendSerial()
{
  if (serialState == SENDING && rxNdx == 0) {        // avoid bus collision, only send when we are not receiving data
    if (mySerial.availableForWrite() > 0 && txNdx == 0) {

      preTransmission();
      
      crc = 0xFFFF;
      mySerial.write(queueHeaders.first().uid);        // send uid (address)
      //debugSerial.print(queueHeaders.first().uid,HEX);debugSerial.print(",");

      calculateCRC(queueHeaders.first().uid);
    }
    while (mySerial.availableForWrite() > 0 && txNdx < queueHeaders.first().PDUlen) {
      mySerial.write(queuePDUs[txNdx]);                // send func and data
      //debugSerial.println(queuePDUs[txNdx],HEX);debugSerial.print(",");

      calculateCRC(queuePDUs[txNdx]);
      txNdx++;
    }
    if (mySerial.availableForWrite() > 1 && txNdx == queueHeaders.first().PDUlen) {
      // In Modbus TCP mode we must add CRC (in Modbus RTU over TCP, CRC is already in queuePDUs)
      if (!enableRtuOverTcp || queueHeaders.first().clientNum == SCAN_REQUEST) {
        mySerial.write(lowByte(crc));                            // send CRC, low byte first
        //debugSerial.println(lowByte(crc),HEX);debugSerial.print(",");

        mySerial.write(highByte(crc));
        //debugSerial.println(highByte(crc),HEX);debugSerial.println("");
      }
      txNdx++;
    }
    if (mySerial.availableForWrite() == SERIAL_TX_BUFFER_SIZE - 1 && txNdx > queueHeaders.first().PDUlen) {
      // wait for last byte (incl. CRC) to be sent from serial Tx buffer
      // this if statement is not very reliable (too fast)
      // Serial.isFlushed() method is needed....see https://github.com/arduino/Arduino/pull/3737
      #ifdef DEBUG
      debugSerial<<"Wrote "<<txNdx+2<<" bytes to serial"<<endl;
      #endif

      txNdx = 0;
      txDelay.sleep(frameDelay);

      serialState = DELAY;
    }
  } else if (serialState == DELAY && txDelay.isOver()) {
    serialTxCount += queueHeaders.first().PDUlen + 1;    // in Modbus RTU over TCP, queuePDUs already contains CRC
    if (!enableRtuOverTcp) serialTxCount += 2;  // in Modbus TCP, add 2 bytes for CRC

    postTransmission();

    if (queueHeaders.first().uid == 0x00) {           // Modbus broadcast - we do not count attempts and delete immediatelly
      serialState = MBIDLE;
      modbusBusy = false;
      deleteRequest();
    } else {
      serialState = WAITING;
      requestTimeout.sleep(serialTimeout);          // delays next serial write
      queueRetries.unshift(queueRetries.shift() + 1);
    }
  }
}

void recvSerial()
{
  static byte serialIn[modbusSize];
  while (mySerial.available() > 0) {
/* 
    //this timeout fires before frameTO and broke good packet processing
    if (rxTimeout.isOver() && rxNdx != 0) {
      rxErr = true;       // character timeout
      #ifdef DEBUG
      debugSerial<<"InterDigit timeout"<<endl;
      #endif
    }
*/
    if (rxNdx < modbusSize) {
      serialIn[rxNdx] = mySerial.read();
      
      //Serial.write(">>");
      //Serial.println(serialIn[rxNdx],HEX);

      rxNdx++;
    } else {
      //debugSerial.write(">!>");
      //debugSerial.println(mySerial.read(),HEX);  
      rxErr = true;       // frame longer than maximum allowed
    }
    rxDelay.sleep(frameDelay);
    rxTimeout.sleep(charTimeout);
  }
  if (rxDelay.isOver() && rxNdx != 0) {
    if (!serialIn[rxNdx-1]) rxNdx--;   /// Raw hack - extra 0 byte received from some controllers 
    // Process Serial data
    // Checks: 1) RTU frame is without errors; 2) CRC; 3) address of incoming packet against first request in queue; 4) only expected responses are forwarded to TCP/UDP
    if (!rxErr && checkCRC(serialIn, rxNdx) == true && serialIn[0] == queueHeaders.first().uid && serialState == WAITING) {
      #ifdef DEBUG
      debugSerial << "Correct packet received from Serial:" << rxNdx << endl;
      #endif
      setSlaveResponding(serialIn[0], true);               // flag slave as responding
      byte MBAP[] = {queueHeaders.first().tid[0], queueHeaders.first().tid[1], 0x00, 0x00, highByte(rxNdx - 2), lowByte(rxNdx - 2)};
      if (queueHeaders.first().clientNum == UDP_REQUEST) {
        Udp.beginPacket(queueHeaders.first().remIP, queueHeaders.first().remPort);
        if (enableRtuOverTcp) Udp.write(serialIn, rxNdx);
        else {
          Udp.write(MBAP, 6);
          Udp.write(serialIn, rxNdx - 2);      //send without CRC
        }
        Udp.endPacket();
        ethTxCount += rxNdx;
        if (!enableRtuOverTcp) ethTxCount += 4;
      } else if (queueHeaders.first().clientNum != SCAN_REQUEST) {
        EthernetClient client = EthernetClient(queueHeaders.first().clientNum);
        // make sure that this is really our socket
        if (client.localPort() == tcpPort && (client.status() == SnSR::ESTABLISHED || client.status() == SnSR::CLOSE_WAIT)) {
          if (enableRtuOverTcp) client.write(serialIn, rxNdx);
          else {
            client.write(MBAP, 6);
            client.write(serialIn, rxNdx - 2);     //send without CRC
            #ifdef DEBUG
            debugSerial << "Packet transmitted to TCP  " << rxNdx << endl;
            #endif
          }
          ethTxCount += rxNdx;
          if (!enableRtuOverTcp) ethTxCount += 4;
        }
      }
      deleteRequest();
      serialState = MBIDLE;
      modbusBusy = false;
    }
    #ifdef DEBUG
    debugSerial << "Packet cleared. " << rxNdx << "bytes"<< endl;
    debugSerial.print(">>");
    for (byte i=0;i<rxNdx;i++)
                {debugSerial.print(serialIn[i],HEX);debugSerial.print(",");}
    debugSerial<<endl;          
    #endif
    serialRxCount += rxNdx;
    rxNdx = 0;
    rxErr = false;
  }

  // Deal with Serial timeouts (i.e. Modbus RTU timeouts)
  if (serialState == WAITING && requestTimeout.isOver()) {
    setSlaveResponding(queueHeaders.first().uid, false);     // flag slave as nonresponding
    if (queueRetries.first() >= serialRetry) {
      // send modbus error 11 (Gateway Target Device Failed to Respond) - usually means that target device (address) is not present
      byte MBAP[] = {queueHeaders.first().tid[0], queueHeaders.first().tid[1], 0x00, 0x00, 0x00, 0x03};
      byte PDU[] = {queueHeaders.first().uid, (byte)(queuePDUs[0] + 0x80), 0x0B};
      crc = 0xFFFF;
      for (byte i = 0; i < sizeof(PDU); i++) {
        calculateCRC(PDU[i]);
      }
      if (queueHeaders.first().clientNum == UDP_REQUEST) {
        Udp.beginPacket(queueHeaders.first().remIP, queueHeaders.first().remPort);
        if (!enableRtuOverTcp) {
          Udp.write(MBAP, 6);
        }
        Udp.write(PDU, 3);
        if (enableRtuOverTcp) {
          Udp.write(lowByte(crc));        // send CRC, low byte first
          Udp.write(highByte(crc));
        }
        Udp.endPacket();
        ethTxCount += 5;
        if (!enableRtuOverTcp) ethTxCount += 4;
      } else {
        EthernetClient client = EthernetClient(queueHeaders.first().clientNum);
        // make sure that this is really our socket
        if (client.localPort() == tcpPort && (client.status() == SnSR::ESTABLISHED || client.status() == SnSR::CLOSE_WAIT)) {
          if (!enableRtuOverTcp) {
            client.write(MBAP, 6);
          }
          client.write(PDU, 3);
          if (enableRtuOverTcp) {
            client.write(lowByte(crc));        // send CRC, low byte first
            client.write(highByte(crc));
          }
          ethTxCount += 5;
          if (!enableRtuOverTcp) ethTxCount += 4;
        }
      }
      deleteRequest();
    }            // if (queueRetries.first() >= MAX_RETRY)
    serialState = MBIDLE;
    modbusBusy = false;
  }              // if (requestTimeout.isOver() && expectingData == true)
}

bool checkCRC(byte buf[], int len)
{
  crc = 0xFFFF;
  for (byte i = 0; i < len - 2; i++) {
    calculateCRC(buf[i]);
  }
  if (highByte(crc) == buf[len - 1] && lowByte(crc) == buf[len - 2]) {
    #ifdef DEBUG  
    debugSerial<<"CRC ok "<<highByte(crc) <<"="<<  buf[len - 1] <<" "<< lowByte(crc)<<"="<<buf[len - 2]<<endl;
    #endif
    return true;
  } else {
    #ifdef DEBUG    
    debugSerial<<"BAD CRC"<<endl; 
    #endif 
    return false;
  }
}

void calculateCRC(byte b)
{
  crc ^= (uint16_t)b;          // XOR byte into least sig. byte of crc
  for (byte i = 8; i != 0; i--) {    // Loop over each bit
    if ((crc & 0x0001) != 0) {      // If the LSB is set
      crc >>= 1;                    // Shift right and XOR 0xA001
      crc ^= 0xA001;
    }
    else                            // Else LSB is not set
      crc >>= 1;                    // Just shift right
  }
  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
}

void ipmodbusLoop()
{
  recvUdp();
  recvTcp();
  processRequests();
  sendSerial();
  recvSerial();
}


void setupIpmodbus(){  
  
  modbusServer = EthernetServer(tcpPort);
  Udp.begin(udpPort);
  modbusServer.begin();
 
   // Calculate Modbus RTU character timeout and frame delay
  byte bits =                                         // number of bits per character (11 in default Modbus RTU settings)
    1 +                                               // start bit
    (((MODBUS_DIMMER_PARAM & 0x06) >> 1) + 5) +  // data bits
    (((MODBUS_DIMMER_PARAM & 0x08) >> 3) + 1);   // stop bits
  if (((MODBUS_DIMMER_PARAM & 0x30) >> 4) > 1) bits += 1;    // parity bit (if present)
  //bits = 11;

  int T = ((unsigned long)bits * 1000000UL) / MODBUS_SERIAL_BAUD;       // time to send 1 character over serial in microseconds
  if (MODBUS_SERIAL_BAUD <= 19200) {
    charTimeout = 1.5 * T;         // inter-character time-out should be 1,5T
    frameDelay = 3.5 * T;         // inter-frame delay should be 3,5T
  }
  else {
    charTimeout = 750;
    frameDelay = 1750;
  }

  //debugSerial<<"Char TO="<<charTimeout<<" frameDelay="<<frameDelay<<endl;
}
#endif