
#pragma once
#include "options.h"
#include "colorchannel.h"
#ifndef SPILED_DISABLE
#include <abstractout.h>
#include <item.h>

#ifdef ADAFRUIT_LED
#include <Adafruit_NeoPixel.h>
#else
#include "FastLED.h"
#endif

/**
 * @brief Addressable LED strip driver, SPI-like data protocol (CH_SPILED = 11).
 *
 * Drives a NeoPixel-compatible LED strip (FastLED or Adafruit_NeoPixel
 * backend) from a single data pin.
 *
 * JSON config (item argument array):
 * @code
 * [11, [data_pin, numLeds, ledsType]]
 * @endcode
 * - data_pin  - the strip data line (default 3);
 * - numLeds   - number of LEDs on the strip (default 43);
 * - ledsType  - LED data order / speed (e.g. NeoPixel flags),
 *   default BRG 800 kHz.
 *
 * The channel type is derived from ledsType: RGBW if the strip has a
 * white channel, RGB otherwise. A subItem "from-to" (or a single LED
 * number) limits the command to an LED range, otherwise the whole
 * strip is affected.
 */
class out_SPILed : public colorChannel {
public:

    //out_SPILed(Item * _item):colorChannel(_item){getConfig();};
    //out_SPILed(){};
    /// Attach to an item and read the strip configuration.
    void link(Item * _item){colorChannel::link(_item);if (_item) getConfig();};
    /// Allocate and initialize the LED strip (FastLED / Adafruit_NeoPixel).
    int Setup() override;
    /// Turn off the strip and free the LED buffer.
    int Stop() override;
    /// Channel type derived from the LED strip type (RGB or RGBW).
    int getChanType() override;
    //int Ctrl(short cmd, short n=0, int * Parameters=NULL,  int suffixCode=0, char* subItem=NULL) override;
    //int Ctrl(itemCmd cmd, char* subItem=NULL) override;
    /// Set the LED range (or the whole strip) to ON/OFF or an RGB color and show the result.
    int PixelCtrl(itemCmd cmd, char* subItem=NULL, bool show=true, bool authorized = false ) override;
    //int PixelCtrl(itemCmd cmd, int from =0 , int to = 1024, bool show = 1) override;
    /// Number of LEDs on the strip.
    int numLeds;
    /// Data pin of the strip.
    int8_t pin;
    /// LED data order / speed flags.
    int ledsType;
protected:
    /// Read data pin, LED count and LED type from the item arguments.
    void getConfig();
};
#endif
