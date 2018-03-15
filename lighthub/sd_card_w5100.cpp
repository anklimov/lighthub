//
// Created by livello on 14.10.17.
//

/*
  SD card test

This example shows how use the utility libraries on which the'
SD library is based in order to get info about your SD card.
Very useful for testing a card when you're not sure whether its working or not.

The circuit:
  * SD card attached to SPI bus as follows:
** MOSI - pin 11 on Arduino Uno/Duemilanove/Diecimila
** MISO - pin 12 on Arduino Uno/Duemilanove/Diecimila
** CLK - pin 13 on Arduino Uno/Duemilanove/Diecimila
** CS - depends on your SD card shield or module.
        Pin 4 used here for consistency with other Arduino examples


created  28 Mar 2011
by Limor Fried
modified 9 Apr 2012
by Tom Igoe
*/

//template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }


#include "sd_card_w5100.h"
#include <SD.h>
#include <SPI.h>
//#include <iostream>

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile sdFile;
File myFile;


char *entireFileContents;

// Arduino Ethernet shield: pin 4
const int chipSelect = 4;

void bench() {
/*    uint8_t buf[BUF_SIZE];
    long maxLatency,minLatency,totalLatency,temp_DHT22;


    myFile = SD.open("test.txt", FILE_WRITE);

    // if the file opened okay, write to it:
    if (myFile) {
        Serial.print("Writing to test.txt...");
        myFile.println("testing 1, 2, 3.");
        // close the file:

    } else {
        // if the file didn'temp_DHT22 open, print an error:
        Serial.println("error opening test.txt");
    }


    for (uint16_t i = 0; i < (BUF_SIZE - 2); i++) {
        buf[i] = 'A' + (i % 26);
    }
    buf[BUF_SIZE - 2] = '\r';
    buf[BUF_SIZE - 1] = '\n';

    Serial.println("File size MB: ");
    Serial.print(FILE_SIZE_MB);
    Serial.println("Buffer size ");
    Serial.print(BUF_SIZE);
    Serial.println("Starting write test, please wait.");

// do write test
    uint32_t n = FILE_SIZE / sizeof(buf);
    Serial.println("write speed and latency");
    Serial.print(" speed,max,min,avg");
    Serial.print(" KB/Sec,usec,usec,usec");
    for (uint8_t nTest = 0; nTest < WRITE_PASS_COUNT; nTest++) {
//        myFile.truncate(0);
        maxLatency = 0;
        minLatency = 9999999;
        totalLatency = 0;
        temp_DHT22 = millis();
        for (uint32_t i = 0; i < n; i++) {
            uint32_t m = micros();
            if (myFile.write(buf, sizeof(buf)) != sizeof(buf)) {
                Serial.println("write failed");
            }
            m = micros() - m;
            if (maxLatency < m) {
                maxLatency = m;
            }
            if (minLatency > m) {
                minLatency = m;
            }
            totalLatency += m;
        }
        myFile.flush();
        temp_DHT22 = millis() - temp_DHT22;
        int s = myFile.size();
        Serial.println(s / temp_DHT22);
        Serial.print(',');
        Serial.print(maxLatency);
        Serial.print(',');
        Serial.print(minLatency);
        Serial.print(',');
        Serial.print(totalLatency / n);
    }*/
    myFile.close();
    Serial.println("done.");
}

char* sdW5100_readEntireFile(const char *filename) {
    SdFile requestedFile;
    if(requestedFile.open(sdFile,filename)) {
        Serial.println("Success open INDEX.HTM:");
        long time_started = millis();
        entireFileContents = new char[requestedFile.fileSize()];
        requestedFile.read(entireFileContents,requestedFile.fileSize());
        Serial.print(millis()-time_started);
        Serial.println(" milliseconds takes to read.");
        return entireFileContents;
        }
    else {
        Serial.print("Failed sdFile.open ");
        Serial.println(filename);

    }
    return NULL;

}
uint32_t sdW5100_getFileSize(const char *filename){
    SdFile requestedFile;
    if(requestedFile.open(sdFile,filename))
        return requestedFile.fileSize();
    else
        return 0;
}

void sd_card_w5100_setup() {
    Serial.print("\nInitializing SD card...");
    // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
    // Note that even if it's not used as the CS pin, the hardware SS pin
    // (10 on most Arduino boards, 53 on the Mega) must be left as an output
    // or the SD library functions will not work.
    pinMode(chipSelect, OUTPUT);     // change this to 53 on a mega
    SPI.begin(chipSelect); //but it didn'temp_DHT22 workt wiht it and whitout it   [/b]  // change this to 53 on a mega

    if (!card.init(SPI_FULL_SPEED, chipSelect)) {
        Serial.println("initialization failed. Things to check:");
        Serial.println("* is a card is inserted?");
        Serial.println("* Is your wiring correct?");
        Serial.println("* did you change the chipSelect pin to match your shield or module?");
        return;
    } else {
        Serial.println("Wiring is correct and a card is present.");
    }

    // print the type of card
    Serial.print("\nCard type: ");
    switch (card.type()) {
        case SD_CARD_TYPE_SD1:
            Serial.println("SD1");
            break;
        case SD_CARD_TYPE_SD2:
            Serial.println("SD2");
            break;
        case SD_CARD_TYPE_SDHC:
            Serial.println("SDHC");
            break;
        default:
            Serial.println("Unknown");
    }

    // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
    if (!volume.init(card)) {
        Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
        return;
    }


    // print the type and size of the first FAT-type volume
    uint32_t volumesize;
    Serial.print("\nVolume type is FAT");
    Serial.println(volume.fatType(), DEC);
    Serial.println();

    volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
    volumesize *= volume.clusterCount();       // we'll have a lot of clusters
    volumesize *= 512;                            // SD card blocks are always 512 bytes
    Serial.print("Volume size (bytes): ");
    Serial.println(volumesize);
    Serial.print("Volume size (Kbytes): ");
    volumesize /= 1024;
    Serial.println(volumesize);
    Serial.print("Volume size (Mbytes): ");
    volumesize /= 1024;
    Serial.println(volumesize);


    Serial.println("\nFiles found on the card (name, date and size in bytes): ");
    sdFile.openRoot(volume);
    // list all files in the card with date and size
    sdFile.ls(LS_R | LS_DATE | LS_SIZE);
    Serial.println(sdW5100_readEntireFile("INDEX.HTM"));
}


