#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

struct EMGData {
  int16_t channelData[8]; // Data for 8 channels
};

// package EMGData structs together for more efficient BLE transmission
struct TransmitData {
  EMGData dataToSend[15];
};

struct InterferenceOutput {
  int8_t pose;
  int16_t probability;
};

#endif