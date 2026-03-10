#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <SPI.h>

struct EMGData {
  uint32_t timestamp; // Timestamp in milliseconds
  int16_t channelData[8]; // Data for 8 channels
};

#endif // MAIN_H