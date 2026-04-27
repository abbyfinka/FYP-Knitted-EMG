#include <Arduino.h>
#include <SPI.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "ADS119X.h"
#include <BLE2902.h>
#include "freertos/ringbuf.h" // change to stream buffer?
#include "main.h"
#include "ADS119X.h"
#include "Filters.h"
#include <Filters/Notch.hpp>

// UUIDs for BLE service and characteristic
#define SERVICE_UUID        "cde33313-b7aa-4b32-b29f-9043b1d8e042"
#define CHARACTERISTIC_UUID "89fea506-0482-4895-b474-843229dae557"

bool sendTestData = false; // Flag to control sending test data instead of real ADS119X data

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

TaskHandle_t handleBLE;  // handle BLE connections
TaskHandle_t handleTransmit;  // handle BLE data transmission
TaskHandle_t handleReadADS = NULL;  // handle reading data from ADS1198

RingbufHandle_t bleDataBufferRaw;
RingbufHandle_t bleDataBuffer; 

const double f_n = 2 * 50 / 1000;
auto notch_filter = simpleNotchFIR(f_n);
bool notchFilter = true;

// Pin definitions
#define CS_           10     // chip select pin
#define DRDY_         9      // data ready pin
#define RESET_        46     // reset pin
#define PWDN_         3      // power down pin
#define START         8      // start pin
#define SDN_          18     // shutdown pin
// #define LOD_PLUS      16
// #define LOD_NEG       17
#define LED_PIN       15      // LED pin for debugging


class MyServerCallbacks: 

public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device has connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device has disconnected");
    }
};

// Initialise ADS119X
ADS119X ADS(DRDY_, RESET_, CS_);

void handleBLETask(void * pvParameters) {
  for (;;) {
    // Disconnecting
    if (!deviceConnected && oldDeviceConnected) {
      delay(500); // Give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // Restart advertising
      Serial.println("Start advertising");
      oldDeviceConnected = deviceConnected;

      // pause EMG reading while no device connected
      //digitalWrite(PWDN_, 0); // change to also turn off clock if concerned about power saving
      digitalWrite(LED_PIN, LOW); // Turn on LED to indicate connection
      
    }
    // Connecting
    if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = deviceConnected;

      // resume EMG reading
      //digitalWrite(PWDN_, 1);
      //delay(10);
      digitalWrite(LED_PIN, HIGH); // Turn on LED to indicate connection
      
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void parseADSDataTask(void * pvParameters) {
  for (;;) {
    size_t itemSize;
    dataPacket* data1 = (dataPacket*)xRingbufferReceive(bleDataBufferRaw, &itemSize, portMAX_DELAY); // Wait for data to be available in the buffer
    int16_t channelData1[8] = {0}; 
    if (data1 != NULL) {

      for (int i = 0; i < 8; i++) {
        int index = i * 2;
        
        // convert 2 uint8_t values to 1 int16_t value for each channel
        if (sendTestData || !notchFilter) {
          channelData1[i] = ((data1->channelData[index] << 8) | data1->channelData[index + 1]) * (2.4 / 32767); // ADC scaling to convert to mV
        } else { // apply notch filter to data
          double convertedData = (data1->channelData[index] << 8) | data1->channelData[index + 1];
          convertedData = convertedData * (2.4 / 32767);
          channelData1[i] = notch_filter(convertedData); 
        }
      }

      EMGData emg = {data1->timestamp, *channelData1}; // Create EMGData struct with timestamp and channel data

      BaseType_t result = xRingbufferSend(bleDataBuffer, &emg, sizeof(EMGData), 0); // Send data to buffer
      vRingbufferReturnItem(bleDataBufferRaw, (void*)data1); // Return the item to the buffer after processing

      // if (result != pdTRUE) {
      //   Serial.println("Failed to send data to buffer.");
      // }

    }
  }
}

void handleTransmitTask(void * pvParameters) {
  for (;;) {
 
    // Serial.println("Checking buffer for data to send...");
    // Serial.println("Data found in buffer, sending over BLE...");
    size_t itemSize;
    EMGData* data1 = (EMGData *)xRingbufferReceive(bleDataBuffer, &itemSize, portMAX_DELAY); // Wait for data to be available in the buffer
    if (data1 != NULL) {

      // EMGData dataToSend = *data; // Get the next data string from the buffer

      TransmitData dataToSend;
      memcpy(&dataToSend.data1, data1, sizeof(EMGData)); // Copy the first EMGData struct into the TransmitData struct

      EMGData* data2 = (EMGData *)xRingbufferReceive(bleDataBuffer, &itemSize, portMAX_DELAY); // Wait for data to be available in the buffer
      if (data2 != NULL){
        memcpy(&dataToSend.data2, data2, sizeof(EMGData)); // Copy the first EMGData struct into the TransmitData struct
      }

      pCharacteristic->setValue((uint8_t*)&dataToSend, sizeof(TransmitData)); // Set the characteristic value
      if (deviceConnected) { pCharacteristic->notify(); } // Notify connected clients
      vRingbufferReturnItem(bleDataBuffer, (void*)data1); // Return the item to the buffer after processing
      vRingbufferReturnItem(bleDataBuffer, (void*)data2); // Return the item to the buffer after processing
    }
    

    //Serial.println(digitalRead(LOD_PLUS));
    //Serial.println(digitalRead(LOD_NEG));
    // Serial.println("No data to send.");
  }
}

void readADSTask(void * pvParameters) {
  for (;;) {
    
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait until notified by the DRDY_ ISR
    // pdTRUE clears missed notifications rather than accumulating in a queue
    
    // if (!ADS.isDRDY()) {
    //     // Serial.println("Data ready signal received, but DRDY_ pin is not low. Skipping data read.");
    //     continue; // Skip to the next loop immediately
    // }

    ADS.readChannelData();
    dataPacket* channelData = ADS.getAllChannelData(); // Get all channel data as a string
    BaseType_t result = xRingbufferSend(bleDataBuffer, channelData, sizeof(dataPacket), 0); // Send data to buffer
    // Serial.println(channelData->channelData[0]);

    // if (result != pdTRUE) {
    //   Serial.println("Failed to send data to buffer.");
    // }
  }
}

void IRAM_ATTR DRDY__ISR() {

  // Serial.println(millis());
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (handleReadADS != NULL) {
    vTaskNotifyGiveFromISR(handleReadADS, &xHigherPriorityTaskWoken); // Notify the task that data is ready
  }

  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR(); // go to readADS task
  }

}

void setup() {

  Serial.begin(115200);

  Serial.println("Starting Setup...");

  bleDataBuffer = xRingbufferCreate(20 * sizeof(EMGData), RINGBUF_TYPE_NOSPLIT); // FreeRTOS ring buffer for BLE data

  if (bleDataBuffer == NULL) {
    Serial.println("Failed to create ring buffer - bleDataBuffer");
    while (true); // Stop execution if buffer creation fails
  }

  bleDataBufferRaw = xRingbufferCreate(20 * sizeof(EMGData), RINGBUF_TYPE_NOSPLIT); // FreeRTOS ring buffer for BLE data

  if (bleDataBufferRaw == NULL) {
    Serial.println("Failed to create ring buffer - bleDataBufferRaw");
    while (true); // Stop execution if buffer creation fails
  }

  Serial.println("Starting BLE setup...");
  //BLE setup
  BLEDevice::init("EMG-Logger");
  pServer = BLEDevice::createServer();  
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID,
                            BLECharacteristic::PROPERTY_READ   |
                            BLECharacteristic::PROPERTY_WRITE  |
                            BLECharacteristic::PROPERTY_NOTIFY |
                            BLECharacteristic::PROPERTY_INDICATE);
  
  pCharacteristic->addDescriptor(new BLE2902());
  
  pCharacteristic->setValue("Initial value");
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Started BLE advertising...");

  // set device pins high to keep devices active (tie high in final design)
  pinMode(PWDN_, OUTPUT);
  pinMode(START, OUTPUT);
  pinMode(SDN_, OUTPUT);
  digitalWrite(PWDN_, HIGH);
  digitalWrite(START, HIGH);
  digitalWrite(SDN_, HIGH);
  
  pinMode(17, OUTPUT);
  digitalWrite(17, HIGH);

  // lead off detection
  // pinMode(LOD_PLUS, INPUT);
  // pinMode(LOD_NEG, INPUT);

  // LED on Test Board
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Turn off LED initially
  
  // setup
  delay(100); // Short delay to ensure pins are set before initializing ADS
  Serial.print(ADS.begin() ? "ADS119X initialized successfully" : "Failed to initialize ADS119X");
  Serial.println();
  delay(100); // Short delay to ensure ADS is ready before configuration

  // send ADS setup commands
  ADS.sendCommand(ADS119X_CMD_SDATAC); // pause data conversion to send config commands
  ADS.setDataRate(ADS119X_DRATE_1000SPS);
  if (!sendTestData) {
    ADS.setAllChannelGain(ADS119X_CHnSET_GAIN_8); // setting gain to 8 for all channels
    // ADS.setAllChannelGain(ADS119X_CHnSET_GAIN_1);
    ADS.setAllChannelMux(ADS119X_CHnSET_MUX_NORMAL); 
    ADS.enableRLD();
  }
  delay(10);
  ADS.startContinuousConversion(); // start reading data continuously


  attachInterrupt(DRDY_, DRDY__ISR, FALLING); // Attach interrupt to DRDY_ pin

  xTaskCreatePinnedToCore(parseADSDataTask, "ParseADS", 10000, NULL, 1, &handleReadADS, 0);
  xTaskCreatePinnedToCore(handleBLETask, "UpdateBLE", 10000, NULL, 2, &handleBLE, 0);
  xTaskCreatePinnedToCore(handleTransmitTask, "Transmit", 10000, NULL, 2, &handleTransmit, 0);
  xTaskCreatePinnedToCore(readADSTask, "ReadADS", 10000, NULL, 1, &handleReadADS, 1);

}

void loop() {
  vTaskDelete(NULL); // Delete loop task
}


