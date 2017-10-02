#define D_UPDATED1 1
#define D_UPDATED2 2
#define D_UPDATED3 4
#define D_UPDATED4 8
#define D_CHECKT 300

#define MAX_CHANNELS 60
//define MAX_IN_CHANNELS 16

//#define DMX_OUT_PIN  3

#include <DmxSimple.h>
#include <Artnet.h>
#include <DMXSerial.h>
#include "aJSON.h"

extern aJsonObject *dmxArr;
extern Artnet *artnet;


void DMXput(void);
void DMXinSetup(int channels);
void ArtnetSetup();
void DMXCheck(void);
int itemCtrl2(char* name,int r,int g, int b, int w);
