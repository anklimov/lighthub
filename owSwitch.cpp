/* Copyright © 2017-2018 Andrey Klimov. (anklimov@gmail.com) All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Homepage: http://lazyhome.ru
GIT:      https://github.com/anklimov/lighthub

*/

#include "owSwitch.h"
#include "owTerm.h"
#include <Arduino.h>
#include "utils.h"

int owRead2408(uint8_t* addr) {
  uint8_t buf[13];
//   PrintBytes(buf, 13, true);
if (!net) return -1;
  net->reset();
  net->select(addr);	
  
//  uint8_t buf[13];  // Put everything in the buffer so we can compute CRC easily.
  buf[0] = 0xF0;    // Read PIO Registers
  buf[1] = 0x88;    // LSB address
  buf[2] = 0x00;    // MSB address
  net->write_bytes(buf, 3,1);
  net->read_bytes(buf+3, 10);     // 3 cmd bytes, 6 data bytes, 2 0xFF, 2 CRC16
  net->reset();

  if (!OneWire::check_crc16(buf, 11, &buf[11])) {
    Serial.print(F("CRC failure in DS2408 at "));
    PrintBytes(addr, 8, true);
    PrintBytes(buf+3,10);
    return -1;
  }
    return (buf[3]);
}


/*
int read1W(int i)
{
  
  Serial.print("1W requested: ");
  Serial.println (i);
  
  int t=-1;  
  switch (term[i][0]){
  case 0x29: // DS2408
    t=owRead2408(term[i]);
   break;
  case 0x28: // Thermomerer  
    t=sensors.getTempC(term[i]);
  }
 return t; 
}
*/


int ow2408out(DeviceAddress addr,uint8_t cur)
{
  if (!net) return -1;
  uint8_t buf[5];
  net->reset();
  net->select(addr);
  buf[0] = 0x5A;     // Write PIO Registers
  buf[1]=cur;
  buf[2] = ~buf[1];     
  net->write_bytes(buf, 3);
  net->read_bytes(buf+3, 2);     
  //net.reset();
  PrintBytes(buf, 5);
    Serial.print(F(" Out: "));Serial.print(buf[1],BIN);
    Serial.print(F(" In: "));Serial.println(buf[4],BIN);
  if (buf[3] != 0xAA) {
      Serial.print(F("Write failure in DS2408 at "));
        PrintBytes(addr, 8, true);
        return -2;
     }  
  return buf[4];   
}
int cntrl2408(uint8_t* addr, int subchan, int val) {
if (!net) return -1;
 
 uint8_t buf;
 int mask,devnum;
 
 if ((devnum=owFind(addr))<0) return -1; 
 buf=regs[devnum];
  Serial.print(F("Current: "));Serial.println(buf,BIN);
  mask=0;
  int r,f;
  switch (subchan) {
  case 0: 
           if ((buf & SW_STAT0) != ((val)?SW_STAT0:0)) 
          {
           if (wstat[devnum] & (SW_PULSE0|SW_PULSE_P0)) 
              {
              wstat[devnum]|=SW_CHANGED_P0;
              Serial.println(F("Rollback 0"));
              } 
           else {
           wstat[devnum]|=SW_PULSE0;
                regs[devnum] = (ow2408out(addr,(buf | SW_MASK) & ~SW_OUT0) & SW_INMASK) ^ SW_STAT0;           ///?
          
           }
          }      
          return 0;
  case 1: 
          if ((buf & SW_STAT1) != ((val)?SW_STAT1:0)) 
          {
           if (wstat[devnum] & (SW_PULSE1|SW_PULSE_P1)) 
              {
              wstat[devnum]|=SW_CHANGED_P1;
              Serial.println(F("Rollback 1"));
              } 
           else {
           wstat[devnum]|=SW_PULSE1;  
                  regs[devnum] =(ow2408out(addr,(buf | SW_MASK) & ~SW_OUT1)  & SW_INMASK) ^ SW_STAT1;         /// -?

           }
          }      
          return 0;
 /* Assume AUX 0&1 it is INPUTS - no write
  case 2: 
           mask=SW_AUX0;  
           break;
  case 3: 
           mask=SW_AUX1; */
  }

  /* Assume AUX 0&1 it is INPUTS - no write
  switch (val) {
  case 0: buf=(buf | SW_MASK | SW_OUT0 | SW_OUT1) | mask;
          break;
  default: buf= (buf | SW_MASK | SW_OUT0 | SW_OUT1) & ~mask;       
  }

 
 regs[devnum] = ow2408out(addr,buf); */
 return 0;
  }


int cntrl2890(uint8_t* addr, int val) {
 uint8_t buf[13];
 if (!net) return -1; 
 // case 0x2C: //Dimmer
     Serial.print(F("Update dimmer "));PrintBytes(addr, 8, true);Serial.print(F(" = "));
     Serial.println(val);
  
  net->reset();
  net->select(addr); 
  
  buf[0] = 0x55;   
  buf[1] = 0x4c;
  net->write_bytes(buf, 2);
  net->read_bytes(buf+2, 1);     // check if  buf[2] == val = ok
  buf[3]=0x96;
  net->write_bytes(buf+3, 1);
  net->read_bytes(buf+4, 1);    // 0 = updated ok
  PrintBytes(buf, 5, true); 

 net->select(addr); 
  
   
if (val==-1)
 {
  buf[0] = 0xF0;    
  net->write_bytes(buf, 1);
  net->read_bytes(buf+1, 2);     // check if  buf[2] == val = ok
  net->reset();
  return buf[2];
 }
else  
 {
  buf[0] = 0x0F;   
  buf[1] = val;
  net->write_bytes(buf, 2);
  net->read_bytes(buf+2, 1);     // check if  buf[2] == val = ok
  buf[3]=0x96;
  net->write_bytes(buf+3, 1);
  net->read_bytes(buf+4, 1);    // 0 = updated ok
  net->reset();
   PrintBytes(buf, 5, true); 
  return buf[2];
 }
  

}

#define DS2413_FAMILY_ID    0x3A
#define DS2413_ACCESS_READ  0xF5
#define DS2413_ACCESS_WRITE 0x5A
#define DS2413_ACK_SUCCESS  0xAA
#define DS2413_ACK_ERROR    0xFF

#define DS2413_IN_PinA    1
#define DS2413_IN_LatchA  2
#define DS2413_IN_PinB    4
#define DS2413_IN_LatchB  8

#define DS2413_OUT_PinA    1
#define DS2413_OUT_PinB     2


/*
byte read(void)
{    
  bool ok = false;
  uint8_t results;

  oneWire.reset();
  oneWire.select(address);
  oneWire.write(DS2413_ACCESS_READ);

  results = oneWire.read();                 / Get the register results   /
  ok = (!results & 0x0F) == (results >> 4); / Compare nibbles            /
  results &= 0x0F;                          / Clear inverted values      /

  oneWire.reset();
  
  // return ok ? results : -1;
  return results;
}

bool write(uint8_t state)
{
  uint8_t ack = 0;
  
  / Top six bits must '1' /
  state |= 0xFC;
  
  oneWire.reset();
  oneWire.select(address);
  oneWire.write(DS2413_ACCESS_WRITE);
  oneWire.write(state);
  oneWire.write(~state);                    / Invert data and resend     /    
  ack = oneWire.read();                     / 0xAA=success, 0xFF=failure /  
  if (ack == DS2413_ACK_SUCCESS)
  {
    oneWire.read();                          / Read the status byte      /
  }
  oneWire.reset();
    
  return (ack == DS2413_ACK_SUCCESS ? true : false);
}

*/



int cntrl2413(uint8_t* addr, int subchan, int val) {
 

 bool ok = false;
 uint8_t results;
 uint8_t cmd;
 uint8_t set=0;
 uint8_t count =10;
 if (!net) return -1;  
 // case 0x85: //Switch
     Serial.print(F("Update switch "));PrintBytes(addr, 8, false); Serial.print(F("/"));Serial.print(subchan);Serial.print(F(" = "));Serial.println(val);
  while (count--)
  {
  net->reset();
  net->select(addr); 
  net->setStrongPullup();
  
  cmd = DS2413_ACCESS_READ;   
  net->write(cmd);
  
  results = net->read();  
  Serial.print(F("Got: ")); Serial.println(results,BIN);
    //Serial.println((~results & 0x0F),BIN); Serial.println ((results >> 4),BIN);
  
  ok = (~results & 0x0F) == (results >> 4); // Compare nibbles            
  results &= 0x0F;                          // Clear inverted values      

  if (ok) {Serial.println(F("Read ok"));break;} else {Serial.println(F("read Error"));delay(1);}
  } //while
  
  if (ok && (val>=0))
  {
  count=10;
  while (count--)
  {
  net->reset();
  net->select(addr); 
  
  if (results & DS2413_IN_LatchA) set|=DS2413_OUT_PinA;
  if (results & DS2413_IN_LatchB) set|=DS2413_OUT_PinB;

  switch (subchan) {
   case 0:
         if (!val) set|=DS2413_OUT_PinA; else set &= ~DS2413_OUT_PinA;
         break;
   case 1:
        if (!val) set|=DS2413_OUT_PinB; else set &= ~DS2413_OUT_PinB;
  };
   set |= 0xFC;
   Serial.print(F("New: "));Serial.println(set,BIN);
   cmd = DS2413_ACCESS_WRITE;   
   net->write(cmd);
  
   net->write(set);
   net->write(~set);
   
   uint8_t ack =  net->read();                   // 0xAA=success, 0xFF=failure
   
  if (ack == DS2413_ACK_SUCCESS)
  {
  results=net->read();  
  Serial.print(F("Updated ok: ")); Serial.println(results,BIN);
  ok = (~results & 0x0F) == (results >> 4); // Compare nibbles     
  {
  if (ok) 
        {Serial.println(F("Readback ok"));
        break;} 
    else {Serial.println(F("readback Error"));delay(1);}    
  }     
  results &= 0x0F;                          // Clear inverted values      
  } 
  else Serial.println (F("Write failed"));;
 
  } //while
  } //if 
return ok ? results : -1;
 

}


int  sensors_ext(void)
{  
 
  int t;  
   switch (term[si][0]){
  case 0x29: // DS2408
    //Serial.println(wstat[si],BIN);
    
    if (wstat[si] & SW_PULSE0) {
      wstat[si]&=~SW_PULSE0;
      wstat[si]|=SW_PULSE_P0;
      Serial.println(F("Pulse0 in progress"));
  
      return 500;
    }

    if (wstat[si] & SW_PULSE0_R) {
      wstat[si]&=~SW_PULSE0_R;
      wstat[si]|=SW_PULSE_P0;
      regs[si] =(ow2408out(term[si],(regs[si] | SW_MASK) & ~SW_OUT0) & SW_INMASK) ^ SW_STAT0; 
      Serial.println(F("Pulse0 in activated"));
  
      return 500;
    }

      if (wstat[si] & SW_PULSE1) {
      wstat[si]&=~SW_PULSE1;
      wstat[si]|=SW_PULSE_P1;
      Serial.println(F("Pulse1 in progress"));
 
      return 500;
    }

    if (wstat[si] & SW_PULSE1_R) {
      wstat[si]&=~SW_PULSE1_R;
      wstat[si]|=SW_PULSE_P1;
      regs[si] =(ow2408out(term[si],(regs[si] | SW_MASK) & ~SW_OUT1) & SW_INMASK) ^ SW_STAT1; 
      Serial.println(F("Pulse0 in activated"));
  
      return 500;
    }

    if (wstat[si] & SW_PULSE_P0) {
      wstat[si]&=~SW_PULSE_P0;
      Serial.println(F("Pulse0 clearing"));
      ow2408out(term[si],regs[si] | SW_MASK | SW_OUT0);
      
      if (wstat[si] & SW_CHANGED_P0) {
        wstat[si]&=~SW_CHANGED_P0;
        wstat[si]|=SW_PULSE0_R;
        return 500;
      }
    }    

if (wstat[si] & SW_PULSE_P1) {
      wstat[si]&=~SW_PULSE_P1;
      Serial.println(F("Pulse1 clearing"));
      ow2408out(term[si],regs[si] | SW_MASK | SW_OUT1);
      
      if (wstat[si] & SW_CHANGED_P1) {
        wstat[si]&=~SW_CHANGED_P1;
        wstat[si]|=SW_PULSE1_R;
        return 500;
      }
    }    
  
    if (wstat[si])
    {
      t=owRead2408(term[si]) & SW_INMASK;
      

    
    if (t!=regs[si]) {
               
        Serial.print(F("DS2408 data = "));
        Serial.println(t, BIN);
        
        if (!(wstat[si] & SW_DOUBLECHECK))
          {
          wstat[si]|=SW_DOUBLECHECK; //suspected
          Serial.println(F("DOUBLECHECK"));
          return recheck_interval;
          }
        
            
            Serial.println(F("Really Changed"));    
            if (owChanged) owChanged(si,term[si],t);    
            regs[si]=t;
    
        
        }
      wstat[si]&=~SW_DOUBLECHECK;   
    }
   break;
   
  
  case 0x01:
  case 0x81:
  t=wstat[si];
  if (t!=regs[si]) 
    { Serial.println(F("Changed")); 
     if (owChanged) owChanged(si,term[si],t);    
     regs[si]=t;
    }
  }
  
  
  si++;
  return check_circle;   
     
}



