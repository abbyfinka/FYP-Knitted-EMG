#ifndef GESTURE_CLASSIFIER_H
#define GESTURE_CLASSIFIER_H

#include "main.h"

#define N_FEATURES              7
#define TRAIN_MEAN              1
#define TRAIN_SD                1
#define EPSILON                 0.01
#define PREDICTION_BUFFER_SIZE  8       // Buffer size for majority voting
#define WINDOW_SIZE             128     // Window size for classification
#define OVERLAP                 0.75    // Amount of overlap between windows
#define FNN_LAYERS              4
#define DATASETS                1
#define N_INPUTS                8 * N_FEATURES
#define N_OUTPUTS               6
#define N_HIDDEN_NEURONS        50

class GestureClassifier
{
    public:
        void extractFeatures(EMGData* frame, float* feature_array);
        float normalise(float data);
        Gesture runInterference(float input_data[]);
    private:

};

#endif // GESTURE_CLASSIFIER_H
