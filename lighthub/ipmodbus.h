/**
 * @file ipmodbus.h
 * @brief IP<->UART Modbus gateway (Modbus TCP/UDP over RS485).
 *
 * Bridges Modbus TCP/UDP clients (port 502) to a Modbus RTU device on a
 * UART (RS485). Incoming TCP/UDP requests are queued, sent serially with
 * inter-frame delays, and the RTU responses are matched back to the
 * originating client by MBAP transaction/unit ID. Based on the
 * arduino-modbus-rtu-tcp-gateway design.
 */
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

/**
 * @brief Metadata for one queued Modbus request, so the RTU response
 * can be routed back to the correct client.
 */
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



/** @brief States of the serial (RTU) transmission sub-state machine. */
enum state : byte
{
  MBIDLE, SENDING, DELAY, WAITING
};



/**
 * @brief Main gateway loop: receives TCP/UDP requests, processes the
 * queue and drives the serial RTU transmission.
 */
void ipmodbusLoop();
/**
 * @brief Initializes the Ethernet server/UDP and the Modbus UART.
 */
void setupIpmodbus();
/**
 * @brief Validates an incoming Modbus TCP/UDP request (MBAP header, and
 * RTU CRC when RTU-over-TCP is used).
 * @param buffer Received frame.
 * @param bufferSize Frame length.
 * @return 0 if the request is valid, otherwise an error code.
 */
byte checkRequest(byte buffer[], unsigned int bufferSize);
/**
 * @brief Updates the running Modbus RTU CRC with one byte.
 * @param b Byte to add to the CRC.
 */
void calculateCRC(byte b);
/**
 * @brief Checks whether a Modbus slave (by address) is currently responding.
 * @param index Slave address.
 * @return true if the slave is marked as responding.
 */
bool getSlaveResponding(const uint8_t index);
/**
 * @brief Verifies the trailing Modbus RTU CRC of a frame.
 * @param buf Frame including the CRC bytes.
 * @param len Frame length.
 * @return true if the CRC is valid.
 */
bool checkCRC(byte buf[], int len);
#endif
