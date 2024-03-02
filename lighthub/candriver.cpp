
#ifdef  CANDRV

#include <candriver.h>
#include <Arduino.h>
#include <main.h>
#include <utils.h>

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
extern canStream CANConfStream; 
extern aJsonObject * root;
extern volatile int8_t configLocked;


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

bool canDriver::requestFrame(uint8_t devId, payloadType _payloadType )
{
    canid_t id;
 datagram_t packet;
 bool       res;


 id.reserve=0;
 id.status=0;
 id.payloadType=_payloadType;  
 id.deviceId=devId;
 id.itemId=0; //CRC?
packet.metric1 =0;
 //memcpy(packet.mac,sysConf.mac,6);

 debugSerial<<("Request frame ")<<_payloadType<<F(" for id ")<<devId<<endl;   
  res=write (id.id,&packet,1);
 if (res) state=canState::FrameRequested; 
    else  state=canState::Error; 
    responseTimer=millisNZ();
 return res;
}

bool canDriver::sendRemoteID(macAddress mac)
{
 canid_t id;
 //datagram_t packet;
  bool       res=false;
 id.deviceId=getIdByMac(mac); //Retrieved controllerID
 if (!id.deviceId) return false;

 id.reserve=0;
 id.status=1; //response
 id.payloadType=payloadType::lookupMAC;  

 id.itemId=200;   //CRC16 of remote config
 //packet.data[0]=1;

 debugSerial<<("Send remote ID")<<endl;   
 res = write (id.id);//,&packet,8);
 if (res) state=canState::Idle; 
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
            controllerId = getMyId();
            ready=true;
            return true;
             }

int canDriver::readFrame()
{
 if (!ready) return -1;
                //STM32 
               #if defined(ARDUINO_ARCH_STM32)
               if (STMCan.read(CAN_RX_msg)) 
               {
                if (CAN_RX_msg.len>8) CAN_RX_msg.len=8;
                memcpy(RXpacket.data, CAN_RX_msg.buf,CAN_RX_msg.len);
                RXlen = CAN_RX_msg.len;
                RXid.id  = CAN_RX_msg.id;
                return RXlen;
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

                
                // only print packet data for non-RTR packets
                RXlen=0;
                while (CAN.available()) {
                    RXpacket.data[RXlen++]=CAN.read();
                    if (RXlen>=8) break;
                }
                RXid.id = CAN.packetId();

                debugSerialPort.println();
                return RXlen;
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

        return -1;
}

void canDriver::Poll()
            {  

if (readFrame()>=0) processPacket( RXid, &RXpacket, RXlen);
//State machine

switch (state)
{
    case canState::MACLookup:
   // case canState::FrameRequested:

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

    case canState::ReadConfig:
    {
    //Blocking read config
    if (configLocked) return; // only in safe moments
    configLocked++;

    infoSerial<<F("Requesting Config from CAN")<<endl;

    CANConfStream.open(controllerId,payloadType::configFrame,'r');

    if (CANConfStream.peek() == '{') {
                debugSerial<<F("JSON detected")<<endl;
                aJsonStream as = aJsonStream(&CANConfStream);
                cleanConf(false);
                root = aJson.parse(&as);
                CANConfStream.close();
                if (!root) {
                    errorSerial<<F("load failed")<<endl;
                    sysConf.setETAG("");
                //    sysConfStream.close();
                    configLocked--;
                    state = canState::Error;
                    return;
                }
                infoSerial<<F("Loaded from CAN")<<endl;
                configLocked--;
                applyConfig();
                sysConf.loadETAG();
                state = canState::Idle;
                return ;
            } 
            CANConfStream.close();   
            infoSerial<<F("Config not loaded")<<endl; 
            state = canState::Error;
            configLocked--;
    }        
    break;

//    case canState::Idle:

}
}

bool canDriver::processPacket(canid_t id, datagram_t *packet, uint8_t len, bool rtr)
{

debugSerial.print("CAN Received ");
debugSerialPort.print(len);
debugSerialPort.print(" bytes id 0x");
debugSerialPort.println(id.id,HEX);

if (len) printFrame(packet,len);
if (id.status)
    //Responces
    switch (state)
    {
        case canState::MACLookup:
        if ((id.payloadType == payloadType::lookupMAC))
           {
                debugSerial<<"Got Controller CAN addr: "<<id.deviceId<<endl;
                controllerId=id.deviceId;
                state = canState::ReadConfig;
           }
        return true; 

        case canState::FrameRequested:
        if ((id.payloadType == payloadType::configFrame) && (id.deviceId == controllerId))
          {
                errorSerial<<F("Config received when not expected")<<endl;
          }

        break;
        case canState::Idle:

        break;

        case canState::Error:
        return false;
    }
else //Requests
    {

        if ((id.payloadType == payloadType::lookupMAC) && (len>=6))
            {             
               return sendRemoteID(packet->mac);
               //debugSerial<<"ID requested"<<endl;
            }
        else if (id.payloadType == payloadType::configFrame)
            {  
                debugSerial<<F("Requested conf for dev#")<<id.deviceId<<endl;
                aJsonObject * remoteConfObj = findConfbyID(id.deviceId);
                if (remoteConfObj)
                {
                        infoSerial<<F("Sending conf for dev#")<<id.deviceId<<endl;
                        CANConfStream.open(id.deviceId,payloadType::configFrame,'w');
                        aJsonStream outStream = aJsonStream(&CANConfStream);
                        aJson.print(remoteConfObj, &outStream); 
                        CANConfStream.close();
                }

               return 1;
               //debugSerial<<"ID requested"<<endl;
            }

    }
return false;
}

uint8_t canDriver::getMyId()
{
if (!root) return 0;    
aJsonObject * canObj =  aJson.getObjectItem(root, "can");
if (!canObj) return 0;

aJsonObject * addrObj = aJson.getObjectItem(canObj, "addr");
if (addrObj && (addrObj->type == aJson_Int)) return addrObj->valueint;

return 0;
}

aJsonObject * canDriver::findConfbyID(uint8_t devId)
{
if (!root) return NULL;    
aJsonObject * canObj =  aJson.getObjectItem(root, "can");
if (!canObj) return NULL;

aJsonObject * remoteConfObj =  aJson.getObjectItem(canObj, "conf");

if (!remoteConfObj) return NULL;
remoteConfObj=remoteConfObj->child; 
while (remoteConfObj)
    {  
        aJsonObject * remoteCanObj =  aJson.getObjectItem(remoteConfObj, "can");
        if (remoteCanObj)
        {
        aJsonObject * addrObj = aJson.getObjectItem(remoteCanObj, "addr");
        if (addrObj && (addrObj->type == aJson_Int) && (addrObj->valueint == devId)) return remoteConfObj;
        }
        remoteConfObj=remoteConfObj->next;
    }
return NULL;    
}


uint8_t canDriver::getIdByMac(macAddress mac)
{
   char macStr[19];
   uint8_t strptr = 0;

   if (!root) return 0;    
   aJsonObject * canObj =  aJson.getObjectItem(root, "can");
   if (!canObj) return 0;
   aJsonObject * confObj =  aJson.getObjectItem(canObj, "conf");
   if (!confObj) return 0;

   memset(macStr,0,sizeof(macStr));
   for (byte i = 0; i < 6; i++)
{
  if (mac[i]<16) macStr[strptr++]='0';

  SetBytes(&mac[i],1,&macStr[strptr]);
  strptr+=2;

   if (i < 5) macStr[strptr++]=':';
}
debugSerial<<F("Searching devId for ")<<macStr<<endl;
aJsonObject * remoteConfObj =  aJson.getObjectItem(confObj, macStr);

if (!remoteConfObj) return 0;

aJsonObject * remoteCanObj = aJson.getObjectItem(remoteConfObj, "can");
 if (!remoteCanObj) return 0;

aJsonObject * addrObj = aJson.getObjectItem(remoteCanObj, "addr");
 if (!addrObj) return 0;
if (addrObj && (addrObj->type == aJson_Int)) 
            {
            debugSerial<<F("find dev#")<< addrObj->valueint << endl;   
            return addrObj->valueint;
            }

return 0;
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
                if (res=STMCan.write(CAN_TX_msg)) debugSerial<<("CAN Wrote ")<<size<<" bytes, id "<<_HEX(msg_id)<<endl;
                    else debugSerial.println("CAN Write error");
                return res;    
                #endif

                //ESP
                #if defined(ARDUINO_ARCH_ESP32)
                CAN.beginExtendedPacket(msg_id,size);
                CAN.write(buf->data,size);
                //for(uint8_t i=0;i<size; i++) CAN.write(buf[i]);
                if (res=CAN.endPacket()) debugSerial<< ("CAN Wrote ")<<size << " bytes, id "<<_HEX(msg_id)<<endl;
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
    




bool canDriver::sendStatus(char * itemName, itemCmd cmd)
{

}
bool canDriver::sendCommand(uint8_t devID, uint16_t itemID, itemCmd cmd)
{

}



////////////////////////////// Steream //////////////////////////


    int canStream::send(uint8_t len)
    {  
                    canid_t id;
                    datagram_t packet;
                    bool       res;
                    if (!driver) return 0;
                    id.reserve=0;
                    id.status=1;
                    id.payloadType=pType;  
                    id.deviceId=devId;
                    id.itemId=0; // chunk?

                    res=driver->write (id.id, &writeBuffer, len);
                    writePos=0;
                    if (res) 
                            {
                            //Await check?    
                            return 1; 
                            }
                       else return 0; 
    }               

     int canStream::checkState() 
     {
      bool res = false;  
      if (!driver) return -1;
        switch (state)
        {
            case canState::StreamOpenedRead:
                       readPos = 0;
                       res= driver->requestFrame(devId,pType); //Requesting frame; 
                       if (res)
                            state = canState::FrameRequested;    
                          else   
                            {
                            state = canState::Error;       
                            return -1;  
                            }              
            //continue 

            case canState::FrameRequested:    
            {
              uint32_t timer = millis();
              int c;
              
              do {
                    debugSerial.print(".");
                    yield();
                    
                    if (c=driver->readFrame()>0 && (driver->RXid.deviceId == devId) && (driver ->RXid.payloadType == pType))   
                           {
                            state = canState::FrameReceived; 
                            debugSerial<<F("Payload received ")<< c << "|" <<driver->RXlen<< " "<<driver->RXpacket.payload<<endl;;
                            return driver->RXlen; 
                           }

                 } while((!isTimeOver(timer,millis(),1000UL)) );

                debugSerial<<F("RX data awaiting timeout")<<endl;
                return -1;
            }    
            break;

            case canState::FrameReceived:
               return driver->RXlen; 
            break;    

            case canState::waitingConfirm:    
              if (driver->readFrame()>=0) 
                        {
                        if (
                            (driver->RXid.deviceId == devId)  &&
                            (driver->RXid.payloadType == pType) &&
                            (driver->RXid.status == 0)
                           )
                        state = canState::StreamOpenedWrite;
                        return 0; 
                        }
              return driver->RXlen; 
            break;

            case canState::Idle:
               return -1;
            break;           
        }
     return -1;
     };



    // Stream methods
     int canStream::available() 
     {
        if (!driver) return -1;
        int avail = checkState();
        return avail;      
     };

     int canStream::read() 
     {  
        if (!driver) return -1;
        int avail = checkState();
        int ch;
       if (avail>=0) 
                {   
                 ch = driver->RXpacket.data[readPos++];
                 if (readPos>=8) state = canState::StreamOpenedRead;
                 return ch;
                }
           else return -1;
     };

     int canStream::peek() 
     {
        if (!driver) return -1;
        int avail = checkState();
        int ch;
       if (avail>=0) 
                {   
                 ch = driver->RXpacket.data[readPos];
                 return ch;
                }
           else return -1;
     };



      size_t canStream::write(uint8_t c) 
    { 
        //if ((state != canState::StreamOpenedWrite) || (state != canState::waitingConfirm)) return -1;

        uint32_t timer = millis();
        do 
         {
         checkState();
         yield();
         //debugSerial.print("*");
         if (isTimeOver(timer,millis(),1000UL))
            {
                 state = canState::Error;
                 errorSerial<<F("CAN write timeout")<<endl;
                 return -1;
            }            

         }
        while (!availableForWrite() );

        writeBuffer.data[writePos++]=c;
        if (writePos>=8) 
            {
                bool res = send(8);
                if (res) state = canState::waitingConfirm;
                    else state = canState::Error;
                return res;
            }
         return 1; };

           void canStream::flush() 
             { 
                send(writePos);
             };


 int canStream::availableForWrite()
{
 switch (state)
 {
    case canState::waitingConfirm: return 0;
 }   
return 1;
}
#endif 
