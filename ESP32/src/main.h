#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <SPI.h>

struct EMGData {
  uint32_t timestamp; // Timestamp in milliseconds
  int16_t channelData[8]; // Data for 8 channels

};

// package two EMGData structs together for more efficient BLE transmission
struct TransmitData {
  EMGData data1;
  EMGData data2;
};



#endif // MAIN_H