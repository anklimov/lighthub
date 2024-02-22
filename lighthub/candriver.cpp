
#ifdef  CANDRV

#include <candriver.h>
#include <Arduino.h>
#include <main.h>

#if defined(ARDUINO_ARCH_STM32)
#include <STM32_CAN.h>
STM32_CAN STMCan( CAN1, ALT, RX_SIZE_64, TX_SIZE_16 );
#endif

#if defined(ARDUINO_ARCH_ESP32)
#include <CAN.h>
#endif
#if defined(__SAM3X8E__)
#include <due_can.h>
#endif

#include <config.h>
//#include <systemconfigdata.h>
extern systemConfig sysConf;


void printFrame(datagram_t * frame, uint8_t len ) {

   debugSerial.print(" Data: 0x");
   for (int count = 0; count < len; count++) {
       debugSerial.print(frame->data[count], HEX);
       debugSerial.print(" ");
   }
   debugSerial.println();
}


bool canDriver::upTime(uint32_t ut)
{
  //  return 0;
 canid_t id;
 datagram_t packet;

 id.reserve=0;
 id.status=1;
 id.payloadType=payloadType::metric;  
 id.deviceId=controllerId;
 id.itemId=metricType::UpTime;

 packet.metric1=ut;

 debugSerial<<("UpTime")<<endl;   
 return write (id.id, &packet, 4);
}

bool canDriver::salt(uint32_t salt)
{
 canid_t id;
 datagram_t packet;

 id.reserve=0;
 id.status=1;
 id.payloadType=payloadType::metric;  
 id.deviceId=controllerId;
 id.itemId=metricType::Salt;

 packet.metric1=salt;

 debugSerial<<("Salt")<<endl;   
 return write (id.id, &packet, 4);
}

bool canDriver::lookupMAC()
{
   // return 0;
    canid_t id;
 datagram_t packet;
 bool       res;

 id.reserve=0;
 id.status=0;
 id.payloadType=payloadType::lookupMAC;  
 id.deviceId=0;
 id.itemId=0; //CRC?

 memcpy(packet.mac,sysConf.mac,6);

 debugSerial<<("Lookup MAC")<<endl;   
  res=write (id.id, &packet, 6);
 if (res) state=canState::MACLookup; 
    else  state=canState::Error; 
    responseTimer=millisNZ();
 return res;
}

bool canDriver::sendRemoteID(macAddress mac)
{
 canid_t id;
 //datagram_t packet;
  bool       res=false;

 id.reserve=0;
 id.status=1; //response
 id.payloadType=payloadType::lookupMAC;  
 id.deviceId=100; //Retrieved controllerID
 id.itemId=200;   //CRC of remote config
 //packet.data[0]=1;

 debugSerial<<("Send remote ID")<<endl;   
 res = write (id.id);//,&packet,8);
 if (res) state=canState::HaveId; 
    else  state=canState::Error; 
//    responseTimer=millisNZ(); ????????
return res;
}


bool canDriver::begin()
            { 
            ready=false;    
            //STM32  
            #if defined(ARDUINO_ARCH_STM32)
            //Can= new STM32_CAN( CAN1, ALT );
            //if (!Can) return false;
            STMCan.begin(); //with retransmission
            STMCan.setBaudRate(125000);
            // STMCan.setFilter( 0, 0x153, 0x1FFFFFFF );
            #endif

            //ESP
            #if defined(ARDUINO_ARCH_ESP32)
            CAN.setPins(GPIO_NUM_35,GPIO_NUM_5);//(rx, tx); 
            // start the CAN bus at 500 kbps
            if (!CAN.begin(125000)) {return false;  }
            #endif    

            #if defined(__SAM3X8E__)
            Can0.begin(CAN_BPS_125K);
            #endif

            debugSerial<<"CAN initialized"<<endl;
            ready=true;
            return true;
             }

void canDriver::Poll()
            {  
               // return ;
                if (!ready) return;
               //STM32 
               #if defined(ARDUINO_ARCH_STM32)
               if (STMCan.read(CAN_RX_msg)) 
               {
               processPacket( CAN_RX_msg.id, (datagram_t*) CAN_RX_msg.buf,CAN_RX_msg.len);
               }

               #endif

            #if defined(ARDUINO_ARCH_ESP32) 
            // try to parse packet
            int packetSize = CAN.parsePacket();

            if (packetSize ){//|| CAN.packetId() != -1) {
                // received a packet
                debugSerialPort.print("Received ");

                if (CAN.packetExtended()) {
                debugSerialPort.print("extended ");
                }

                if (CAN.packetRtr()) {
                // Remote transmission request, packet contains no data
                debugSerialPort.print("RTR ");
                }

                debugSerialPort.print("packet with id 0x");
                debugSerialPort.print(CAN.packetId(), HEX);

                if (CAN.packetRtr()) {
                debugSerialPort.print(" and requested length ");
                debugSerialPort.println(CAN.packetDlc());
                } else {
                debugSerialPort.print(" and length ");
                //debugSerialPort.println(packetSize);
                debugSerialPort.println(CAN.packetDlc());

                datagram_t packet;
                // only print packet data for non-RTR packets
                int i=0;
                while (CAN.available()) {
                    packet.data[i++]=CAN.read();
                    //debugSerialPort.print((char)CAN.read());
                    if (i>=8) break;
                }
                debugSerialPort.println();

                processPacket( CAN.packetId(), &packet,i);
                }
               }
                #endif

                //DUE
                #if defined(__SAM3X8E__)
                CAN_FRAME incoming;
                if (Can0.available() > 0) {
                                                Can0.read(incoming); 
                                                printFrame(incoming);
                                            }

                #endif


//State machine

switch (state)
{
    case canState::MACLookup:
    if (isTimeOver(responseTimer,millis(),1000UL)) 
                            {
                            responseTimer=millisNZ();
                            state=canState::Error;
                            errorSerial<<"CAN Timeout"<<endl;
                            }
    break;

    case canState::Error:
        if (isTimeOver(responseTimer,millis(),10000UL)) 
        lookupMAC();
    break;

//    case canState::HaveId:

}

}

bool canDriver::processPacket(uint32_t rawid, datagram_t *packet, uint8_t len, bool rtr)
{
canid_t id;
id.id = rawid;
debugSerial.print("CAN Received ");
debugSerialPort.print(len);
debugSerialPort.print(" bytes id 0x");
debugSerialPort.println(id.id,HEX);

printFrame(packet,len);
if (id.status)
    //Responces
    switch (state)
    {
        case canState::MACLookup:
        if ((id.payloadType == payloadType::lookupMAC) && (len>=6))
           {
                debugSerial<<"Got Controller CAN addr: "<<id.deviceId<<endl;
                controllerId=id.deviceId;
           }
        return true; 
        case canState::HaveId:
        break;

        case canState::Error:
        return false;
    }
else //Requests


    {

        if (id.payloadType == payloadType::lookupMAC)
            {             
               return sendRemoteID(packet->mac);
               //debugSerial<<"ID requested"<<endl;
            }


    }
return false;
}


bool canDriver::write(uint32_t msg_id, datagram_t * buf, uint8_t size)
            {   //return 0;
                if (!ready) errorSerial<<"CAN not initialized"<<endl;
                bool res;
                if (size>8) size = 8;

                //STM32 
                #if defined(ARDUINO_ARCH_STM32)
                //if (!Can) return 0;
                if (buf) for(uint8_t i=0;i<size; i++) CAN_TX_msg.buf[i]=buf->data[i];
                CAN_TX_msg.id = msg_id;
                CAN_TX_msg.flags.extended = 1;  // To enable extended ID
                CAN_TX_msg.len=size;
                if (res=STMCan.write(CAN_TX_msg)) debugSerial<<("CAN Wrote ")<<size<<" bytes"<<endl;
                    else debugSerial.println("CAN Write error");
                return res;    
                #endif

                //ESP
                #if defined(ARDUINO_ARCH_ESP32)
                CAN.beginExtendedPacket(msg_id,size);
                CAN.write(buf->data,size);
                //for(uint8_t i=0;i<size; i++) CAN.write(buf[i]);
                if (res=CAN.endPacket()) debugSerial.println("CAN Wrote");
                    else debugSerial.println("CAN Write error");
                return res;
                #endif

                #if defined(__SAM3X8E__)
                CAN_FRAME outGoting;
	            outgoing.id = 0x400;
                outgoing.extended = true;
                outgoing.priority = 4; //0-15 lower is higher priority
                
                outgoing.data.s0 = 0xFEED;
                outgoing.data.byte[2] = 0xDD;
                outgoing.data.byte[3] = 0x55;
                outgoing.data.high = 0xDEADBEEF;
                Can0.sendFrame(outgoing);
                #endif
        }
    

#endif 
