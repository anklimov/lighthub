//
// Created by livello on 14.10.17.
//

#include <stdint.h>

#ifndef NODEMCU_STOPWATCH_SD_CARD_W5100_H
#define NODEMCU_STOPWATCH_SD_CARD_W5100_H

#endif //NODEMCU_STOPWATCH_SD_CARD_W5100_H
void sd_card_w5100_setup();
void cidDmp();
void bench();
char* sdW5100_readEntireFile(const char *filename);
uint32_t sdW5100_getFileSize(const char *filename);