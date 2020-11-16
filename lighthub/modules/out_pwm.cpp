#include "modules/out_pwm.h"
#include "Arduino.h"
#include "options.h"
#include "Streaming.h"

#include "item.h"
#include "main.h"
#include "dmx.h"

static int driverStatus = CST_UNKNOWN;

int  out_pwm::Setup()
{
debugSerial<<F("PWM-Out Init")<<endl;
driverStatus = CST_INITIALIZED;
return 1;
}

int  out_pwm::Stop()
{
debugSerial<<F("PWM-Out stop")<<endl;
driverStatus = CST_UNKNOWN;
return 1;
}

int  out_pwm::Status()
{
return driverStatus;
}

int out_pwm::isActive()
{
itemArgStore st;
st.aslong = item->getVal(); //Restore old params
debugSerial<< F(" val:")<<st.v<<endl;
return st.v;
}

int out_pwm::Poll(short cause)
{
return 0;
};

int out_pwm::getChanType()
{
  if (item) return item->itemType;
  return 0;
}

int out_pwm::PixelCtrl(itemCmd cmd)
{
if (!item) return 0;
int iaddr = item->getArg(0);
itemCmd st(ST_RGB);
st.assignFrom(cmd);

    switch (getChanType())
    { case CH_PWM:
//      DmxWrite(iaddr + 3, cmd.getPercents255());
      break;

      default: ;
    }
return 1;
}
