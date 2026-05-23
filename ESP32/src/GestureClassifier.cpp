#include "main.h"
#include "GestureClassifier.h"
#include <math.h>
#include <aifes.h>
#include "weights.h"

// aifes tutorial: https://projecthub.arduino.cc/aifes_team/aifes-express-tutorial-feedforward-neural-network-float32-6cbb7e
// https://www.hackster.io/aifes_team/aifes-inference-tutorial-f44d96

// # inputs -> # neurons in hidden layer -> outputs
uint32_t FNN_structure[FNN_LAYERS] = {N_INPUTS, N_HIDDEN_NEURONS, N_HIDDEN_NEURONS, N_OUTPUTS};
// activation function at each layer
AIFES_E_activations FNN_activations[FNN_LAYERS - 1] = {AIfES_E_relu, AIfES_E_relu};

// AIfES Express struct 
AIFES_E_model_parameter_fnn_f32 FNN;

uint16_t input_shape[2] = {DATASETS, (uint16_t)FNN_structure[0]};
uint16_t output_shape[2] = {1, (uint16_t)FNN_structure[FNN_LAYERS - 1]}; 

float GestureClassifier::normalise(float data)
{
    return (data - TRAIN_MEAN) / TRAIN_SD;
}

void GestureClassifier::extractFeatures(EMGData* frame, float features[]) 
{

    float iemg[8], msv[8], var[8], rms[8], ln_rms[8], kurt[8], skew[8], arm[8];

    // Serial.printf("Start %lu\n", micros());
    for (int i = 0; i < WINDOW_SIZE; i++)
    {

        for (int c = 0; c < 8; c++)
        {
            // Integrated EMG (IEMG)   
            iemg[c] = iemg[c] + fabs(normalise(frame[i].channelData[c]));

            // Mean Squared Value (MSV)
            msv[c] = msv[c] + (normalise(frame[i].channelData[c]) * normalise(frame[i].channelData[c]));

            // Variance
            var[c] = var[c] + fabs(normalise(frame[i].channelData[c]) - TRAIN_MEAN) * fabs(normalise(frame[i].channelData[c]) - TRAIN_MEAN);

            // Root Mean Square (RMS)
            rms[c] = rms[c] + normalise(frame[i].channelData[c]) * normalise(frame[i].channelData[c]);

            // Kurtosis
            kurt[c] = kurt[c] + pow(((normalise(frame[i].channelData[c]) - TRAIN_MEAN) / TRAIN_SD), 4);

            // Skewness
            skew[c] = skew[c] + pow(((normalise(frame[i].channelData[c]) - TRAIN_MEAN) / TRAIN_SD), 3);
        }
    }
    // Serial.printf("End %lu\n", micros());

    // return features: iemg, mse, variance, rms, ln(rms), kurtosis, skewness

    // assigning features to array
    for (int c = 0; c < 8; c++) 
    {
        features[c] = iemg[c]; 
        features[c + 8] = msv[c] / WINDOW_SIZE;
        features[c + 16] = var[c] / WINDOW_SIZE;
        features[c + 24] = sqrt(rms[c] / WINDOW_SIZE);
        features[c + 32] = log(sqrt(rms[c] / WINDOW_SIZE));
        features[c + 40] = (kurt[c] / WINDOW_SIZE) - 3;
        features[c + 48] = skew[c] / WINDOW_SIZE;
    }

}

Gesture GestureClassifier::runInterference(float input_data[])
{
    float output_data[(uint16_t)FNN_structure[FNN_LAYERS - 1]]; 
    aitensor_t output_tensor = AITENSOR_2D_F32(output_shape, output_data);
    aitensor_t input_tensor = AITENSOR_2D_F32(input_shape, input_data); 

    AIFES_E_inference_fnn_f32(&input_tensor,&FNN,&output_tensor);    

    Gesture currentGesture = {0};
    for (int i = 1; i < (uint16_t)FNN_structure[FNN_LAYERS - 1]; i++) {
        if (output_data[i] > output_data[currentGesture.gesture]) {
            currentGesture.gesture = i;
            currentGesture.probability = output_data[i];
        }
    }

    return currentGesture;
}


