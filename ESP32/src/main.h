#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <SPI.h>

#define NUM_OF_PACKETS 10

struct EMGData {
  uint32_t timestamp; // Timestamp in milliseconds
  float channelData[8]; // Data for 8 channels

};

// package EMGData structs together for more efficient BLE transmission
struct TransmitData {
  EMGData dataToSend[NUM_OF_PACKETS];
};



#endif // MAIN_H