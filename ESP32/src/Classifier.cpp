#include "main.h"
#include "Classifier.h"
#include <math.h>
#include <aifes.h>
#include "weights.h"

// aifes tutorial: https://projecthub.arduino.cc/aifes_team/aifes-express-tutorial-feedforward-neural-network-float32-6cbb7e
// https://www.hackster.io/aifes_team/aifes-inference-tutorial-f44d96

void Classifier::initialise()
{       
    FNN.layer_count = FNN_LAYERS; 
    FNN.fnn_structure = FNN_structure; 
    FNN.fnn_activations = FNN_activations; 
    FNN.flat_weights = (void *)WEIGHTS; 
    
}

float Classifier::normalise(float data, int channel)
{
    return (data - TRAIN_MEAN[channel]) / TRAIN_SD[channel];
}

void Classifier::extractFeatures(EMGData frame[], float features[]) 
{

    float iemg[8], msv[8], var[8], rms[8], ln_rms[8], kurt[8], skew[8], arm[8];

    //int startTime = micros();
    for (int i = 0; i < WINDOW_SIZE; i++)
    {

        for (int c = 0; c < 8; c++)
        {
            float data = frame[i].channelData[c] / 1000.0f;
            // Integrated EMG (IEMG)   
            iemg[c] = iemg[c] + fabs(normalise(data, c));

            // Mean Squared Value (MSV)
            msv[c] = msv[c] + (normalise(data, c) * normalise(data, c));

            // Variance
            var[c] = var[c] + fabs(normalise(data, c) - TRAIN_MEAN[c]) * fabs(normalise(data,c) - TRAIN_MEAN[c]);

            // Root Mean Square (RMS)
            rms[c] = rms[c] + normalise(data,c) * normalise(data,c);

            // Kurtosis
            kurt[c] = kurt[c] + pow(((normalise(data,c) - TRAIN_MEAN[c]) / TRAIN_SD[c]), 4);

            // Skewness
            skew[c] = skew[c] + pow(((normalise(data,c) - TRAIN_MEAN[c]) / TRAIN_SD[c]), 3);
        }
    }

    // assigning features to array
    for (int c = 0; c < 8; c++) 
    {
        features[c * 8 + 0] = iemg[c];
        features[c * 8 + 1] = msv[c] / WINDOW_SIZE;
        features[c * 8 + 2] = var[c] / WINDOW_SIZE;
        features[c * 8 + 3] = sqrt(rms[c] / WINDOW_SIZE);
        features[c * 8 + 4] = log(sqrt(rms[c] / WINDOW_SIZE));
        features[c * 8 + 5] = (kurt[c] / WINDOW_SIZE) - 3;
        features[c * 8 + 6] = skew[c] / WINDOW_SIZE;
    }

    //Serial.printf("End %lu\n", (micros()-startTime));
    

}

InterferenceOutput Classifier::runInterference(float* input_data)
{
    
    float output_data[(uint16_t)FNN_structure[FNN_LAYERS - 1]]; 
    aitensor_t output_tensor = AITENSOR_2D_F32(output_shape, output_data);
    aitensor_t input_tensor = AITENSOR_2D_F32(input_shape, input_data); 

    
    int8_t error = 0; 
    error = AIFES_E_inference_fnn_f32(&input_tensor,&FNN,&output_tensor);
    // Serial.println(error);
    
    // finding output with highest probability
    InterferenceOutput currentOut = {0};
    for (int i = 0; i < 6; i++) {
        // Serial.println(output_data[i]);
        if (output_data[i] > output_data[currentOut.pose]) {
            currentOut.pose = (int8_t)i;
            currentOut.probability = (int16_t)(output_data[i] * 100);
        }
    }

    return currentOut;

}


