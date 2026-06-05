#include <Arduino.h>
#include <SPI.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "ADS119X.h"
#include <BLE2902.h>
#include "freertos/ringbuf.h" // change to stream buffer?
#include "main.h"
#include "Classifier.h"
#include "ADS119X.h"
#include <Filters/BiQuad.hpp>
#include "EMA.h"
#include <aifes.h>

#define TEST_DATA_EN            0       // Flag to control sending test data instead of real ADS119X data
#define NOTCH_EN                1       // Flag to control whether to apply the notch filter
#define BANDPASS_EN             1       // Flag to control whether to apply bandpass filter
#define CLASSIFY                1

BLEServer* pServer = NULL;
BLECharacteristic* pDataCharacteristic = NULL;
BLECharacteristic* pGestureCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Task handles
TaskHandle_t handleBLE;               // handle BLE connections
TaskHandle_t handleTransmit;          // handle BLE data transmission
TaskHandle_t handleReadADS = NULL;    // handle reading data from ADS1198
TaskHandle_t handleParseADS;          // converting data to float and filtering
TaskHandle_t handleFeature;           // feature extraction
TaskHandle_t handleInterference;      // running interference

// Buffers
RingbufHandle_t bleDataBufferRaw;     // buffer for raw ADS1198 data
RingbufHandle_t bleDataBuffer;        // buffer for data that needs to be sent over BLE
RingbufHandle_t processingBuffer;     // buffer for data to be classified
RingbufHandle_t featureBuffer;        // buffer of feature vectors
RingbufHandle_t votingBuffer;         // buffer to implement majority voting

float input[N_FEATURES];

// initialising a filter for each channel
BiQuadFilterDF1<float> biquad_notch_filter[8];
BiQuadFilterDF1<float> biquad_hp_filter[8];
BiQuadFilterDF1<float> biquad_lp_filter[8];
EMA<float> ema[8];

EMGData currentWindow[WINDOW_SIZE] = {0};
EMGData nextWindow[WINDOW_SIZE] = {0};

// Pin definitions TESTPCB
#define CS_           10     // chip select pin
#define DRDY_         9      // data ready pin
#define RESET_        46     // reset pin
#define PWDN_         3      // power down pin
#define START         8      // start pin
#define SDN_          18     // shutdown pin
#define LED_PIN       15     // LED pin for debugging

// Pin definitions FINALPCB
// #define DRDY_         9      // data ready pin
// #define RESET_        8     // reset pin
// #define LED_PIN       15     // LED pin for debugging

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

ADS119X ADS(DRDY_, RESET_); // Initialise ADS119X
Classifier gc; // Initialise classifier

void handleBLETask(void * pvParameters) 
{
  for (;;) 
  {
    // Disconnecting
    if (!deviceConnected && oldDeviceConnected) 
    {
      delay(500); // Give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // Restart advertising
      Serial.println("Start advertising");
      oldDeviceConnected = deviceConnected;

      digitalWrite(LED_PIN, LOW); // Turn on LED to indicate connection
    }
    // Connecting
    if (deviceConnected && !oldDeviceConnected) 
    {
      oldDeviceConnected = deviceConnected;

      digitalWrite(LED_PIN, HIGH); // Turn on LED to indicate connection
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void parseADSDataTask(void * pvParameters) 
{
  for (;;) 
  {
    EMGData emg = {0};
    size_t itemSize;
    dataPacket* data1 = (dataPacket*)xRingbufferReceive(bleDataBufferRaw, &itemSize, portMAX_DELAY); // Wait for data to be available in the buffer
    int16_t channelData[8] = {0};
    if (data1 != NULL) 
    {
      for (int i = 0; i < 8; i++) 
      {
        int index = i * 2;
        
        int16_t rawData = ((int16_t)(data1->channelData[index] << 8) | (int16_t)data1->channelData[index + 1]); // combining two 8 bit values from ADC into 16 bit value for each channel
        
        float convertedData = rawData * (2.4f / 32767.0f) * 1000.0f; // scaled ADC values to mV
        
        // real time filtering

        if (NOTCH_EN && !TEST_DATA_EN) 
        {
          convertedData = biquad_notch_filter[i](convertedData); // applying notch filter
        }

        if (BANDPASS_EN && !TEST_DATA_EN)
        {
          // Exponential Moving Average filter to remove fluctuating DC offset
          convertedData = ema[i].filter(convertedData);

          // applying biquad filters
          convertedData = biquad_hp_filter[i](convertedData); // applying highpass filter
          convertedData = biquad_lp_filter[i](convertedData); // applying lowpass filter
        }
        
        channelData[i] = (int16_t)(convertedData * 1000);
      }

      // Creating instance of EMGData from next set of data
      // emg.timestamp = data1->timestamp;
      // emg.timestamp = 0;
      memcpy(&emg.channelData, channelData, sizeof(channelData));

      // Add data to buffer for BLE transfer
      xRingbufferSend(bleDataBuffer, &emg, sizeof(EMGData), 0);

      // Adding data to buffer for classification
      xRingbufferSend(processingBuffer, &emg, sizeof(EMGData), 0);

      // Return the item to the buffer after processing
      vRingbufferReturnItem(bleDataBufferRaw, (void*)data1); 
    }

  }
}

void handleTransmitTask(void * pvParameters) 
{
  for (;;) 
  {
    TransmitData dataToSend = {0};
    for (int i = 0; i < N_PACKETS; i++) 
    {
      size_t itemSize;
      EMGData* data = (EMGData *)xRingbufferReceive(bleDataBuffer, &itemSize, portMAX_DELAY); // Wait for data to be available in the buffer
      if (data != NULL) 
      {
        memcpy(&dataToSend.dataToSend[i], data, sizeof(EMGData)); // Copy data to the TransmitData struct
        vRingbufferReturnItem(bleDataBuffer, (void*)data); // Return the item to the buffer after processing
      }
    }

    pDataCharacteristic->setValue((uint8_t*)&dataToSend, sizeof(TransmitData)); // Set the characteristic value
    if (deviceConnected && !CLASSIFY) { pDataCharacteristic->notify(); } // Notify connected clients

  }
}

void handleFeatureExtractionTask(void * pvParameters)
{
  for (;;) 
  {
    for (int i = 0; i < WINDOW_SIZE; i++) 
    {
      if (i < WINDOW_SIZE * OVERLAP) 
      { 
        nextWindow[i] = currentWindow[static_cast<int>(WINDOW_SIZE * (1.0 - OVERLAP) + i)];
      }
      else
      {
        size_t itemSize;
        EMGData* data = (EMGData *)xRingbufferReceive(processingBuffer, &itemSize, portMAX_DELAY); // Wait for data to be available in the buffer
        if (data != NULL) 
        {
          nextWindow[i] = *data;
          vRingbufferReturnItem(processingBuffer, (void*)data); // Return the item to the buffer after processing
        }
      }
    }

    // verifying windows
    // for (int i = 0; i < WINDOW_SIZE; i++)
    // {
    //   Serial.print(nextWindow[i].channelData[3]);
    //   Serial.print(", ");
    //   Serial.println(" ");
    // }

    memcpy(currentWindow, nextWindow, sizeof(EMGData)); // Copy next window to current window
    float features[N_FEATURES * 8] = {0};
    gc.extractFeatures(currentWindow, features);
    
    // Adding data to buffer for classification
    xRingbufferSend(featureBuffer, &features, sizeof(float) * N_FEATURES, 0);

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void handleClassificationTask(void * pvParameters)
{
  for (;;){

    InterferenceOutput classification = {0};
    size_t itemSize;
    float* features = (float *)xRingbufferReceive(featureBuffer, &itemSize, portMAX_DELAY); // Wait for data to be available in the buffer
    if (features != NULL) 
    {
      classification = gc.runInterference(features);
      vRingbufferReturnItem(featureBuffer, (void*)features); // Return the item to the buffer after processing

      if (classification.probability > 50){
        xRingbufferSend(votingBuffer, &classification.pose, sizeof(int8_t), 0);
      }
      // Serial.println(classification.pose);
      // Serial.println(classification.probability);
      pGestureCharacteristic->setValue((uint8_t*)&classification, sizeof(classification)); // Set the characteristic value
      if (deviceConnected) { pGestureCharacteristic->notify(); } // Notify connected clients
    }
    vTaskDelay(pdMS_TO_TICKS(10));

  }
}


void readADSTask(void * pvParameters) 
{
  for (;;) 
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait until notified by the DRDY_ ISR
    // pdTRUE clears missed notifications rather than accumulating in a queue

    ADS.readChannelData();
    dataPacket* channelData = ADS.getAllChannelData(); // Get all channel data
    xRingbufferSend(bleDataBufferRaw, channelData, sizeof(dataPacket), 0); // Send data to buffer
  }
}

void IRAM_ATTR DRDY__ISR() 
{
  // Serial.println(millis());
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (handleReadADS != NULL) 
  {
    vTaskNotifyGiveFromISR(handleReadADS, &xHigherPriorityTaskWoken); // Notify the task that data is ready
  }

  if (xHigherPriorityTaskWoken) 
  {
    portYIELD_FROM_ISR(); // go to readADS task
  }

}

void setup() 
{

  Serial.begin(115200);
  Serial.println("Starting Setup...");

  // -------------------- Initialising filters -------------------- //

  if (!TEST_DATA_EN)
  {
    if (NOTCH_EN) 
    {
      // Initialising a notch filter for each channel with calculated coefficients
      for (int i = 0; i < 8; i++)
      {
        biquad_notch_filter[i] = BiQuadFilterDF1<float>({NB0, NB1, NB2}, {NA0, NA1, NA2});
      }
    }
    
    if (BANDPASS_EN) 
    {
      // initialising high pass and low pass filters with calculated coefficients
      for (int i = 0; i < 8; i++)
      {
        ema[i].begin(EMA_ALPHA);
        biquad_hp_filter[i] = BiQuadFilterDF1<float>({HB0, HB1, HB2}, {HA0, HA1, HA2});
        biquad_lp_filter[i] = BiQuadFilterDF1<float>({LB0, LB1, LB2}, {LA0, LA1, LA2});
      }
      
    }
  }

  // -------------------- Creating buffers -------------------- //

  bleDataBuffer = xRingbufferCreate(BLE_BUFFER_SIZE * sizeof(EMGData), RINGBUF_TYPE_NOSPLIT); // FreeRTOS ring buffer for BLE data

  if (bleDataBuffer == NULL) 
  {
    Serial.println("Failed to create ring buffer - bleDataBuffer");
    while (true); // Stop execution if buffer creation fails
  }

  bleDataBufferRaw = xRingbufferCreate(INPUT_DATA_BUFFER * sizeof(EMGData), RINGBUF_TYPE_NOSPLIT); // FreeRTOS ring buffer for BLE data

  if (bleDataBufferRaw == NULL) 
  {
    Serial.println("Failed to create ring buffer - bleDataBufferRaw");
    while (true); // Stop execution if buffer creation fails
  }

  processingBuffer = xRingbufferCreate(WINDOW_SIZE * 5 * sizeof(EMGData), RINGBUF_TYPE_NOSPLIT);

  if (bleDataBufferRaw == NULL) 
  {
    Serial.println("Failed to create ring buffer - bleDataBufferRaw");
    while (true); // Stop execution if buffer creation fails
  }

  featureBuffer = xRingbufferCreate(N_FEATURES * 8 * 5 * sizeof(float), RINGBUF_TYPE_NOSPLIT);

  if (bleDataBufferRaw == NULL) 
  {
    Serial.println("Failed to create ring buffer - bleDataBufferRaw");
    while (true); // Stop execution if buffer creation fails
  }

  votingBuffer = xRingbufferCreate(5 * sizeof(int8_t), RINGBUF_TYPE_NOSPLIT);

  if (votingBuffer == NULL)
  {
    Serial.println("Failed to create ring buffer - votingBuffer");
    while (true);
  }

  // -------------------- BLE setup -------------------- //

  Serial.println("Starting BLE setup...");
  BLEDevice::init("EMG-Logger");
  BLEDevice::setMTU(517); // allowing to send bigger data packets
  pServer = BLEDevice::createServer();  
  pServer->setCallbacks(new MyServerCallbacks());

  // Initialise service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Initialise characteristic that holds most recent data
  pDataCharacteristic = pService->createCharacteristic(DATA_CHARACTERISTIC_UUID,
                                                        BLECharacteristic::PROPERTY_READ   |
                                                        BLECharacteristic::PROPERTY_WRITE  |
                                                        BLECharacteristic::PROPERTY_NOTIFY |
                                                        BLECharacteristic::PROPERTY_INDICATE);
  pDataCharacteristic->addDescriptor(new BLE2902());
  pDataCharacteristic->setValue("Initial value");
  
  // Initialise characteristic that holds most recent gesture
  pGestureCharacteristic = pService->createCharacteristic(GESTURE_CHARACTERISTIC_UUID,
                                                        BLECharacteristic::PROPERTY_READ   |
                                                        BLECharacteristic::PROPERTY_WRITE  |
                                                        BLECharacteristic::PROPERTY_NOTIFY |
                                                        BLECharacteristic::PROPERTY_INDICATE);
  pGestureCharacteristic->addDescriptor(new BLE2902());
  pGestureCharacteristic->setValue("Initial value");

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Started BLE advertising...");

  // -------------------- Pin Setup -------------------- //

  // required pin setup for TESTPCB
  pinMode(PWDN_, OUTPUT);
  pinMode(START, OUTPUT);
  pinMode(SDN_, OUTPUT);
  pinMode(CS_, OUTPUT);
  digitalWrite(PWDN_, HIGH);
  digitalWrite(START, HIGH);
  digitalWrite(SDN_, HIGH);
  digitalWrite(CS_, LOW);
  
  // required pin setup for breadboard testing
  // pinMode(17, OUTPUT);
  // digitalWrite(17, HIGH);

  // LED on Test Board
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Turn off LED initially
  
  // -------------------- ADS Initialisation -------------------- //

  delay(100); // Short delay to ensure pins are set before initializing ADS
  Serial.print(ADS.begin() ? "ADS119X initialized successfully" : "Failed to initialize ADS119X");
  Serial.println();
  delay(100); // Short delay to ensure ADS is ready before configuration

  // send ADS setup commands
  ADS.sendCommand(ADS119X_CMD_SDATAC); // pause data conversion to send config commands
  ADS.setDataRate(ADS119X_DRATE_1000SPS);
  ADS.setAllChannelGain(ADS119X_CHnSET_GAIN_1);
  if (!TEST_DATA_EN) {
    ADS.setAllChannelGain(ADS119X_CHnSET_GAIN_12); // setting gain to 12 for all channels
    // ADS.setAllChannelGain(ADS119X_CHnSET_GAIN_6);
    ADS.setAllChannelMux(ADS119X_CHnSET_MUX_NORMAL); 
    ADS.enableRLD();
  }
  delay(10);
  ADS.startContinuousConversion(); // start reading data continuously

  // -------------------- AIfES Setup -------------------- //
  gc.initialise();

  // -------------------- RTOS Tasks Setup -------------------- //

  attachInterrupt(DRDY_, DRDY__ISR, FALLING); // Attach interrupt to DRDY_ pin

  xTaskCreatePinnedToCore(parseADSDataTask, "ParseADS", 5000, NULL, 1, &handleParseADS, 0);
  xTaskCreatePinnedToCore(handleBLETask, "UpdateBLE", 5000, NULL, 2, &handleBLE, 0);
  xTaskCreatePinnedToCore(handleTransmitTask, "Transmit", 5000, NULL, 2, &handleTransmit, 0);
  xTaskCreatePinnedToCore(readADSTask, "ReadADS", 5000, NULL, 1, &handleReadADS, 1);
  if (CLASSIFY){
    xTaskCreatePinnedToCore(handleFeatureExtractionTask, "FeatureExtraction", 10000, NULL, 1, &handleFeature, 0);
    xTaskCreatePinnedToCore(handleClassificationTask, "Classify", 10000, NULL, 2, &handleInterference, 0);
  }
  

}

void loop() {
  vTaskDelete(NULL); // Delete loop task
}


