#include <Arduino.h>
#include <SPI.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "ADS119X.h"
#include <BLE2902.h>
#include "freertos/ringbuf.h"
#include "main.h"

// UUIDs for BLE service and characteristic
#define SERVICE_UUID        "cde33313-b7aa-4b32-b29f-9043b1d8e042"
#define CHARACTERISTIC_UUID "89fea506-0482-4895-b474-843229dae557"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

TaskHandle_t Task1;  // handle BLE connections
TaskHandle_t Task2;  // handle BLE data transmission
TaskHandle_t drdyTask = NULL;  // handle reading data from ADS1198

RingbufHandle_t bleDataBuffer;

// Pin definitions
#define CS      10     // chip select pin
#define DRDY    9      // data ready pin
#define RST     46     // reset pin
#define PWDN    3      // power down pin
#define START   8      // start pin
#define SDN     18     // shutdown pin

volatile bool dataReady = false; // Flag to indicate data ready from ADS1198

class MyServerCallbacks: 

public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      // Serial.println("Device has connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      // Serial.println("Device has disconnected");
    }
};

// Initialise ADS119X
ADS119X ADS(DRDY, RST, CS);

void handleBLETask(void * pvParameters) {
  for (;;) {
    // Disconnecting
    if (!deviceConnected && oldDeviceConnected) {
      delay(500); // Give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // Restart advertising
      Serial.println("Start advertising");
      oldDeviceConnected = deviceConnected;
    }
    // Connecting
    if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = deviceConnected;
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
      pCharacteristic->notify(); // Notify connected clients
      vRingbufferReturnItem(bleDataBuffer, data); // Return the item to the buffer after processing
    }
    
    // Serial.println("No data to send.");
  }
}

void readADSTask(void * pvParameters) {
  for (;;) {
    
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait until notified by the DRDY ISR

    if (!ADS.isDRDY()) {
        // Serial.println("Data ready signal received, but DRDY pin is not low. Skipping data read.");
        continue; // Skip to the next loop immediately
    }

    EMGData channelData = ADS.getAllChannelData(); // Get all channel data as a string
    BaseType_t result = xRingbufferSend(bleDataBuffer, &channelData, sizeof(EMGData), 0); // Send data to buffer
    
    if (result != pdTRUE) {
      Serial.println("Failed to send data to buffer.");
    }
  }
}

void IRAM_ATTR DRDY_ISR() {

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (drdyTask != NULL) {
    vTaskNotifyGiveFromISR(drdyTask, &xHigherPriorityTaskWoken); // Notify the task that data is ready
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
  pinMode(PWDN, OUTPUT);
  pinMode(START, OUTPUT);
  pinMode(SDN, OUTPUT);
  digitalWrite(PWDN, 1);
  digitalWrite(START, 1);
  digitalWrite(SDN, 1);
  
  Serial.println(ADS.begin() == true ? "ADS1198 initialized successfully!" : "ADS1198 initialization failed!");
  ADS.setAllChannelGain(ADS119X_CHnSET_GAIN_8); // setting gain to 8 for all channels
  ADS.sendCommand (ADS119X_CMD_SDATAC);
  ADS.setAllChannelGain( ADS119X_CHnSET_GAIN_1);  
  ADS.setAllChannelMux(ADS119X_CHnSET_MUX_NORMAL); 
  ADS.setDataRate(ADS119X_DRATE_1000SPS);
  ADS.enableRLD();
  ADS.startContinuousConversion();

  attachInterrupt(DRDY, DRDY_ISR, FALLING); // Attach interrupt to DRDY pin

  xTaskCreatePinnedToCore(handleBLETask, "UpdateBLE", 10000, NULL, 1, &Task1, 0);
  xTaskCreatePinnedToCore(handleTransmitTask, "Transmit", 10000, NULL, 1, &Task2, 0);
  xTaskCreatePinnedToCore(readADSTask, "ReadADS", 10000, NULL, 1, &drdyTask, 1);

}

void loop() {
  vTaskDelete(NULL); // Delete loop task
}


