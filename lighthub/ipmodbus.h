#ifdef IPMODBUS
#pragma once
#include <CircularBuffer.h>     // CircularBuffer https://github.com/rlogiacco/CircularBuffer
#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
//#include <utility/w5100.h>
#include <HardwareSerial.h>
#include "options.h"
#include "microtimer.h"
//#include "streamlog.h"
//extern Streamlog  debugSerial;
#include "main.h"
extern void preTransmission();
extern void postTransmission();
extern short modbusBusy;


#ifndef SERIAL_TX_BUFFER_SIZE
#define SERIAL_TX_BUFFER_SIZE SERIAL_BUFFER_SIZE
#endif

const bool enableRtuOverTcp = false;
const int serialRetry = 5;
const int serialTimeout = 800;
const int tcpPort = 502;
const int udpPort = 502;

typedef struct {
  byte tid[2];            // MBAP Transaction ID
  byte uid;               // MBAP Unit ID (address)
  byte PDUlen;            // lenght of PDU (func + data) stored in queuePDUs
  IPAddress remIP;        // remote IP for UDP client (UDP response is sent back to remote IP)
  unsigned int remPort;   // remote port for UDP client (UDP response is sent back to remote port)
  byte clientNum;         // TCP client who sent the request, UDP_REQUEST (0xFF) designates UDP client
} header;



const byte reqQueueCount = 15;       // max number of TCP or UDP requests stored in queue
const int reqQueueSize = 256;        // total length of TCP or UDP requests stored in queue (in bytes)
const byte maxSlaves = 247;          // max number of Modbus slaves (Modbus supports up to 247 slaves, the rest is for reserved addresses)
const int modbusSize = 256;          // size of a MODBUS RTU frame (determines size of serialInBuffer and tcpInBuffer)

#define mySerial modbusSerial              // define serial port for RS485 interface, for Arduino Mega choose from Serial1, Serial2 or Serial3         

// #define DEBUG            // Main Serial (USB) is used for printing some debug info, not for Modbus RTU. At the moment, only web server related debug messages are printed.
//#define debugSerial Serial

#ifdef MAX_SOCK_NUM           //if the macro MAX_SOCK_NUM is defined 
// #undef MAX_SOCK_NUM           //un-define it
// #define MAX_SOCK_NUM 8        //redefine it with the new value
#endif 

/****** ETHERNET AND SERIAL ******/

#ifdef UDP_TX_PACKET_MAX_SIZE               //if the macro MAX_SOCK_NUM is defined 
#undef UDP_TX_PACKET_MAX_SIZE               //un-define it
#define UDP_TX_PACKET_MAX_SIZE modbusSize   //redefine it with the new value
#endif 




//#ifdef DEBUG
#define dbg(x...) debugSerial.print(x);
#define dbgln(x...) debugSerial.println(x);
//#else /* DEBUG */
//#define dbg(x...) ;
//#define dbgln(x...) ;
//#endif /* DEBUG */

#define UDP_REQUEST 0xFF      // We store these codes in "header.clientNum" in order to differentiate 
#define SCAN_REQUEST 0xFE      // between TCP requests (their clientNum is nevew higher than 0x07), UDP requests and scan requests (triggered by scan button)



enum state : byte
{
  MBIDLE, SENDING, DELAY, WAITING
};



void ipmodbusLoop();
void setupIpmodbus();
byte checkRequest(byte buffer[], unsigned int bufferSize);
void calculateCRC(byte b);
bool getSlaveResponding(const uint8_t index);
bool checkCRC(byte buf[], int len);
#endif
