#ifndef CLASSIFIER_H
#define CLASSIFIER_H

#include "main.h"
#include "aifes.h"

#define N_FEATURES              7
#define EPSILON                 0.01
#define PREDICTION_BUFFER_SIZE  8       // Buffer size for majority voting
#define WINDOW_SIZE             128     // Window size for classification
#define OVERLAP                 0.75    // Amount of overlap between windows
#define FNN_LAYERS              3
#define N_INPUTS                8 * N_FEATURES
#define N_OUTPUTS               6
#define N_HIDDEN_NEURONS        50

class Classifier
{
    public:
        void extractFeatures(EMGData* frame, float* feature_array);
        float normalise(float data, int channel);
        InterferenceOutput runInterference(float* input_data);
        void initialise();

    private:

        // AIfES Express struct 
        AIFES_E_model_parameter_fnn_f32 FNN;
        // # inputs -> # neurons in hidden layer -> outputs
        uint32_t FNN_structure[FNN_LAYERS] = {N_INPUTS, N_HIDDEN_NEURONS, N_OUTPUTS};
        // activation function at each layer
        AIFES_E_activations FNN_activations[FNN_LAYERS - 1] = {AIfES_E_relu, AIfES_E_softmax};
        
        uint16_t input_shape[2] = {1, (uint16_t)FNN_structure[0]};
        uint16_t output_shape[2] = {1, (uint16_t)FNN_structure[FNN_LAYERS - 1]}; 

};

#endif // GESTURE_CLASSIFIER_H
