#include "unity.h"
#include "Classifier.h"
#include "test-data.h"

Classifier gc;

void test_iemg_extration(void) {
  float features[N_FEATURES * 8] = {0};
  gc.extractFeatures(TestFrame, features);

  char msg[50];
  for (int i = 0; i < 8; i++){
    snprintf(msg, sizeof(msg), "Failed at index i = %d", i);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(2, TrueFeatures[i], features[i], msg);
  }
}

void test_msv_extraction(void) {
  float features[N_FEATURES * 8] = {0};
  gc.extractFeatures(TestFrame, features);

  char msg[50];
  for (int i = 0; i < 8; i++){
    snprintf(msg, sizeof(msg), "Failed at index i = %d", i+1);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.05, TrueFeatures[i+1], features[i+1], msg);
  }
}

void test_var_extraction(void) {
  float features[N_FEATURES * 8] = {0};
  gc.extractFeatures(TestFrame, features);

  char msg[50];
  for (int i = 0; i < 8; i++){
    snprintf(msg, sizeof(msg), "Failed at index i = %d", i+2);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.05, TrueFeatures[i+2], features[i+2], msg);
  }
}

void test_rms_extraction(void) {
  float features[N_FEATURES * 8] = {0};
  gc.extractFeatures(TestFrame, features);

  char msg[50];

  for (int i = 0; i < 8; i++){
    snprintf(msg, sizeof(msg), "Failed at index i = %d", i);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.05, TrueFeatures[i+3], features[i+3], msg);
  }
}

void test_ln_rms_extraction(void) {
  float features[N_FEATURES * 8] = {0};
  gc.extractFeatures(TestFrame, features);

  char msg[50];
  for (int i = 0; i < 8; i++){
    snprintf(msg, sizeof(msg), "Failed at index i = %d", i);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.05, TrueFeatures[i+4], features[i+4], msg);
  }
}

void test_kurt_extraction(void) {
  float features[N_FEATURES * 8] = {0};
  gc.extractFeatures(TestFrame, features);

  char msg[50];
  for (int i = 0; i < 8; i++){
    snprintf(msg, sizeof(msg), "Failed at index i = %d", i);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.05, TrueFeatures[i+5], features[i+5], msg);
  }
}

void test_skew_extraction(void) {
  float features[N_FEATURES * 8] = {0};
  gc.extractFeatures(TestFrame, features);

  char msg[50];
  for (int i = 0; i < 8; i++){
    snprintf(msg, sizeof(msg), "Failed at index i = %d", i);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.05, TrueFeatures[i+6], features[i+6], msg);
  }
}

int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_iemg_extration);
  RUN_TEST(test_msv_extraction);
  RUN_TEST(test_var_extraction);
  RUN_TEST(test_rms_extraction);
  RUN_TEST(test_ln_rms_extraction);
  RUN_TEST(test_kurt_extraction);
  RUN_TEST(test_skew_extraction);
  return UNITY_END();
}

void setup() {
  // Wait ~2 seconds before the Unity test runner
  // establishes connection with a board Serial interface
  Serial.begin(115200);
  delay(2000);
  gc.initialise();
  runUnityTests();
}

void loop() {}
