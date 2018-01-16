/* Copyright © 2017-2018 Andrey Klimov. All rights reserved.

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
e-mail    anklimov@gmail.com

*/

#include "inputs.h"
#include "aJSON.h"
#include "item.h"
#include <PubSubClient2.h>

extern PubSubClient client;

Input::Input(char * name) //Constructor
{
  if (name) 
       inputObj= aJson.getObjectItem(inputs, name);
  else inputObj=NULL;
    
     Parse();  
}


Input::Input(int pin) //Constructor
{
 // TODO
}


 Input::Input(aJsonObject * obj) //Constructor
{ 
  inputObj= obj;  
  Parse();
    
}


boolean Input::isValid ()
{
 return  (pin && store); 
}

void Input::Parse()
{ 
   store         = NULL;
   inType        = 0;
   pin           = 0; 
  
  if (inputObj && (inputObj->type==aJson_Object))
  {
  aJsonObject * s;
  
  s             = aJson.getObjectItem(inputObj,"T");  
  if (s) inType = s->valueint;
  
  pin           = atoi(inputObj->name);

  
  s             = aJson.getObjectItem(inputObj,"S");
  if (!s)     { Serial.print("In: ");Serial.print(pin);Serial.print("/");Serial.println(inType);
                aJson.addNumberToObject(inputObj,"S", 0);
                s = aJson.getObjectItem(inputObj,"S");
                }

  if (s)        store= (inStore *) &s->valueint;
  }
}

int Input::Pool ()
{ 
  boolean v;
  if (!isValid()) return -1;

  
  if (inType & IN_ACTIVE_HIGH) 
      {   pinMode(pin, INPUT);    
        v = (digitalRead(pin)==HIGH); 
      } 
        else 
      {   pinMode(pin, INPUT_PULLUP);    
        v = (digitalRead(pin)==LOW);
      }  
  if (v!=store->cur) // value changed 
      {
            if (store->bounce) store->bounce--;  
               else //confirmed change
               {
                Changed(v);
                store->cur=v;
               }
      }
  else // no change 
      store->bounce=3;         
 return  0; 
}

void Input::Changed (int val)
{
  Serial.print(pin);Serial.print("=");Serial.println(val); 
  aJsonObject * item = aJson.getObjectItem(inputObj,"item");  
  aJsonObject * scmd = aJson.getObjectItem(inputObj,"scmd");
  aJsonObject * rcmd = aJson.getObjectItem(inputObj,"rcmd");   
  aJsonObject * emit = aJson.getObjectItem(inputObj,"emit");    

  if (emit)
  {

  if (val)
            {  //send set command
               if (!scmd) client.publish(emit->valuestring,"ON"); else  if (strlen(scmd->valuestring)) client.publish(emit->valuestring,scmd->valuestring); 
            }
       else
            {  //send reset command
              if (!rcmd) client.publish(emit->valuestring,"OFF");  else  if (strlen(rcmd->valuestring)) client.publish(emit->valuestring,rcmd->valuestring);
            } 
  }

  if (item)
  {
  Item it(item->valuestring);
  if (it.isValid())
      {
       if (val)
            {  //send set command
               if (!scmd) it.Ctrl(CMD_ON,0,NULL,true); else if   (strlen(scmd->valuestring)) it.Ctrl(txt2cmd(scmd->valuestring),0,NULL,true); 
            }
       else
            {  //send reset command
               if (!rcmd) it.Ctrl(CMD_OFF,0,NULL,true); else if  (strlen(rcmd->valuestring)) it.Ctrl(txt2cmd(rcmd->valuestring),0,NULL,true);      
            }
      }
  }
}
