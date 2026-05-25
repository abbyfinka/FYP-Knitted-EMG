#ifndef GESTURE_CLASSIFIER_H
#define GESTURE_CLASSIFIER_H

#include "main.h"
#include "aifes.h"

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
        void initialise();

    private:

        // AIfES Express struct 
        AIFES_E_model_parameter_fnn_f32 FNN;
        // # inputs -> # neurons in hidden layer -> outputs
        uint32_t FNN_structure[FNN_LAYERS] = {N_INPUTS, N_HIDDEN_NEURONS, N_HIDDEN_NEURONS, N_OUTPUTS};
        // activation function at each layer
        AIFES_E_activations FNN_activations[FNN_LAYERS - 1] = {AIfES_E_relu, AIfES_E_relu};
        
        uint16_t input_shape[2] = {DATASETS, (uint16_t)FNN_structure[0]};
        uint16_t output_shape[2] = {1, (uint16_t)FNN_structure[FNN_LAYERS - 1]}; 

};

#endif // GESTURE_CLASSIFIER_H
