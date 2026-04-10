#include <Arduino.h>
#include <SPI.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "ADS119X.h"
#include <BLE2902.h>
#include "freertos/ringbuf.h" // change to stream buffer?
#include "main.h"

// UUIDs for BLE service and characteristic
#define SERVICE_UUID        "cde33313-b7aa-4b32-b29f-9043b1d8e042"
#define CHARACTERISTIC_UUID "89fea506-0482-4895-b474-843229dae557"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

TaskHandle_t handleBLE;  // handle BLE connections
TaskHandle_t handleTransmit;  // handle BLE data transmission
TaskHandle_t handleReadADS = NULL;  // handle reading data from ADS1198

RingbufHandle_t bleDataBuffer;

// Pin definitions
#define CS_        10     // chip select pin
#define DRDY_     9      // data ready pin
#define RESET_       46     // reset pin
#define PWDN_      3      // power down pin
#define START     8      // start pin
#define SDN_      18     // shutdown pin
#define LOD_PLUS  16
#define LOD_NEG   17
#define LED_PIN   15      // LED pin for debugging

// PIN DEFINITIONS - MATCHES PCB DESIGN

// #define LED_PIN   4      // LED pin for debugging
// #define DRDY_     5      // data ready pin

// // GND
// // SCLK           12
// // DIN (MOSI)     11
// // DOUT (MISO)    13
// #define CS_       10     // chip select pin

// #define START     15     // start pin
// #define RESET_    16     // reset pin
// #define PWDN_     17     // power down pin
// #define SDN_      18     // shutdown pin
// #define LOD_PLUS  8
// #define LOD_NEG   3

volatile bool dataReady = false; // Flag to indicate data ready from ADS1198

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
      digitalWrite(PWDN_, 0); // change to also turn off clock if concerned about power saving
      vTaskSuspend(handleTransmit);
      vTaskSuspend(handleReadADS);
      digitalWrite(LED_PIN, LOW); // Turn on LED to indicate connection
      
    }
    // Connecting
    if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = deviceConnected;

      // resume EMG reading
      digitalWrite(PWDN_, 1);
      delay(10);
      vTaskResume(handleTransmit);
      vTaskResume(handleReadADS);
      digitalWrite(LED_PIN, HIGH); // Turn on LED to indicate connection
      
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void handleTransmitTask(void * pvParameters) {
  for (;;) {

    // Serial.println("Checking buffer for data to send...");
    // Serial.println("Data found in buffer, sending over BLE...");
    size_t itemSize;
    void *data = xRingbufferReceive(bleDataBuffer, &itemSize, portMAX_DELAY); // Wait for data to be available in the buffer
    if (data != NULL) {
      // Serial.println("Data found in buffer, sending over BLE...");
      EMGData dataToSend = *(EMGData*)data; // Get the next data string from the buffer
      pCharacteristic->setValue((uint8_t*)&dataToSend, sizeof(EMGData)); // Set the characteristic value
      if (deviceConnected) { pCharacteristic->notify(); } // Notify connected clients
      vRingbufferReturnItem(bleDataBuffer, data); // Return the item to the buffer after processing
    }
    //Serial.println(digitalRead(LOD_PLUS));
    //Serial.println(digitalRead(LOD_NEG));
    // Serial.println("No data to send.");
  }
}

void readADSTask(void * pvParameters) {
  for (;;) {
    
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait until notified by the DRDY_ ISR

    if (!ADS.isDRDY()) {
        // Serial.println("Data ready signal received, but DRDY_ pin is not low. Skipping data read.");
        continue; // Skip to the next loop immediately
    }

    EMGData channelData = ADS.getAllChannelData(); // Get all channel data as a string
    BaseType_t result = xRingbufferSend(bleDataBuffer, &channelData, sizeof(EMGData), 0); // Send data to buffer
    
    if (result != pdTRUE) {
      Serial.println("Failed to send data to buffer.");
    }
  }
}

void IRAM_ATTR DRDY__ISR() {

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (handleReadADS != NULL) {
    vTaskNotifyGiveFromISR(handleReadADS, &xHigherPriorityTaskWoken); // Notify the task that data is ready
  }

  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR(); // Yield to the higher priority task immediately
  }

}

void setup() {

  Serial.begin(115200);

  bleDataBuffer = xRingbufferCreate(20 * sizeof(EMGData), RINGBUF_TYPE_NOSPLIT); // FreeRTOS ring buffer for BLE data

  if (bleDataBuffer == NULL) {
    Serial.println("Failed to create ring buffer");
    while (true); // Stop execution if buffer creation fails
  }

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
  Serial.println("Characteristic defined.");

  // set device pins high to keep devices active (tie high in final design)
  pinMode(PWDN_, OUTPUT);
  pinMode(START, OUTPUT);
  pinMode(SDN_, OUTPUT);
  digitalWrite(PWDN_, 1);
  digitalWrite(START, 1);
  digitalWrite(SDN_, 0); // TEMPORARILY SET LOW

  // lead off detection
  pinMode(LOD_PLUS, INPUT);
  pinMode(LOD_NEG, INPUT);

  // turn on LED
  pinMode(LED_PIN, OUTPUT);
  // digitalWrite(LED_PIN, HIGH);
  // delay(500);
  // digitalWrite(LED_PIN, LOW);
  
  // setup
  delay(100); // Short delay to ensure pins are set before initializing ADS
  Serial.print(ADS.begin() ? "ADS119X initialized successfully" : "Failed to initialize ADS119X");
  Serial.println();
  delay(100); // Short delay to ensure ADS is ready before configuration

  // send ADS setup commands
  ADS.sendCommand(ADS119X_CMD_SDATAC); // pause data conversion to send config commands
  delay(10);
  ADS.setAllChannelGain(ADS119X_CHnSET_GAIN_8); // setting gain to 8 for all channels
  ADS.setAllChannelMux(ADS119X_CHnSET_MUX_NORMAL); 
  ADS.setDataRate(ADS119X_DRATE_1000SPS);
  ADS.enableRLD();
  ADS.startContinuousConversion(); // start reading data continuously

  attachInterrupt(DRDY_, DRDY__ISR, FALLING); // Attach interrupt to DRDY_ pin

  xTaskCreatePinnedToCore(handleBLETask, "UpdateBLE", 10000, NULL, 1, &handleBLE, 0);
  xTaskCreatePinnedToCore(handleTransmitTask, "Transmit", 10000, NULL, 1, &handleTransmit, 0);
  xTaskCreatePinnedToCore(readADSTask, "ReadADS", 10000, NULL, 1, &handleReadADS, 1);

}

void loop() {
  vTaskDelete(NULL); // Delete loop task
}


