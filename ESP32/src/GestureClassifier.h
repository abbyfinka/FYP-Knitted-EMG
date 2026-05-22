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


class GestureClassifier{
    public:
    
        void extractFeatures(EMGData* frame, float* feature_array);
        float normalise(float data);
        float runInference(EMGData* frame, float* aifes_input_buffer);
        aimodel_t *init_neural_network();

    private:
        


};

#endif // GESTURE_CLASSIFIER_H
