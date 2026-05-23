#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <SPI.h>

#define N_PACKETS          10
#define BLE_BUFFER_SIZE         20      // Buffer size for transmission over BLE
#define INPUT_DATA_BUFFER       20      // Buffer size input from the ADS1198

// UUIDs for BLE service and characteristic
#define SERVICE_UUID                "cde33313-b7aa-4b32-b29f-9043b1d8e042"
#define DATA_CHARACTERISTIC_UUID    "89fea506-0482-4895-b474-843229dae557"
#define GESTURE_CHARACTERISTIC_UUID "9122613f-3d96-4ba2-9bb5-382cbda24f02"

// 50 Hz notch, Q = 30
const float n_b0 = 0.98478425f;
const float n_b1 = -1.87317095f;
const float n_b2 = 0.98478425f;
const float n_a0 = 1.0f; // normalised
const float n_a1 = -1.87317095f;
const float n_a2 = 0.96956849f;

// high pass 499 Hz cutoff, Q = 10
const float h_b0 = 0.98985427f;
const float h_b1 = -1.97970854f;
const float h_b2 = 0.98985427f;
const float h_a0 = 1.0f; // normalised
const float h_a1 = -1.97187235f;
const float h_a2 = 0.98754473f;

// low pass 20 Hz cutoff, Q = 10
const float l_b0 = 0.99967607f;
const float l_b1 = -1.99935215f;
const float l_b2 = 0.99967607f;
const float l_a0 = 1.0f; // normalised
const float l_a1 = -1.99933242f;
const float l_a2 = 0.99937188f;

struct EMGData {
  uint32_t timestamp; // Timestamp in milliseconds
  float channelData[8]; // Data for 8 channels

};

// package EMGData structs together for more efficient BLE transmission
struct TransmitData {
  EMGData dataToSend[N_PACKETS];
};

struct Gesture {
  int gesture;
  float probability;
};


#endif // MAIN_H