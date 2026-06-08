#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

#define N_PACKETS               15
#define BLE_BUFFER_SIZE         30      // Buffer size for transmission over BLE
#define INPUT_DATA_BUFFER       20      // Buffer size input from the ADS1198

// UUIDs for BLE service and characteristic
#define SERVICE_UUID                "cde33313-b7aa-4b32-b29f-9043b1d8e042"
#define DATA_CHARACTERISTIC_UUID    "89fea506-0482-4895-b474-843229dae557"
#define GESTURE_CHARACTERISTIC_UUID "9122613f-3d96-4ba2-9bb5-382cbda24f02"

#define EMA_ALPHA 0.5

// 50 Hz notch, Q = 30
#define NB0     0.99487611f
#define NB1     -1.89236681f
#define NB2     0.99487611f
#define NA0     1.0f // normalised
#define NA1     -1.89236681f
#define NA2     0.98975221f

// 100 Hz notch, Q = 30
#define NB0_2   0.99029862f
#define NB1_2   -1.60233682f
#define NB2_2   0.99029862f
#define NA0_2   1.0f
#define NA1_2   -1.60233682
#define NA2_2   0.98059724

// 150 Hz notch
#define NB0_3   0.98669577f
#define NB1_3   -1.15993045f
#define NB2_3   0.98669577f
#define NA0_3   1.0f
#define NA1_3   -1.15993045f
#define NA2_3   0.97339155f

// 200 Hz notch
#define NB0_4   0.98439639f
#define NB1_4   -0.60839043f
#define NB2_4   0.98439639f
#define NA0_4   1.0f
#define NA1_4   -0.60839043f
#define NA2_4   0.96879278f

// 250 Hz notch
#define NB0_5   0.98360656f
#define NB1_5   0.0f
#define NB2_5   0.98360656f
#define NA0_5   1.0f
#define NA1_5   0.0f
#define NA2_5   0.96721311f

// high pass 499 Hz cutoff, Q = 10
#define HB0     0.91495789f
#define HB1     -1.82991579f
#define HB2     0.91495789f
#define HA0     1.0f // normalised
#define HA1     -1.82267251f
#define HA2     0.83715906f

// low pass 20 Hz cutoff, Q = 10
#define LB0    0.99556631f
#define LB1    1.99113261f
#define LB2    0.99556631f
#define LA0    1.0f // normalised
#define LA1    1.99111296f
#define LA2    0.99115227f



#endif // MAIN_H