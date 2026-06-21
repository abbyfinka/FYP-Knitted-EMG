# Development of a Flexible EMG Readout Board for Wearable Hand Movement Prediction

* Final Year Project
* MEng Electrical and Electronic Engineering
* Imperial College London
* Supervisor: Professor Kristel Fobelets

This project aimed to design a low-cost, compact sEMG acquisition device to interface into a knitted armband, using knitted electrodes to acquire sEMG signals. The device achieves a high-sampling rate, with sufficient resolution, and robust wireless transmission with no dropped packets. An embedded classification pipeline was developed using an MLP architecture, aiming to classify six hand/wrist movements. The embedded implementation was not fully tested, however the model was validated using data collected from an alternative acquisition device, the Cyton Board. 

This repository contains all of the code developed in this project, including the PCB files of the hardware developed. 

## Repository Structure

This repo contains:
- `\Hardware`: Contains PCB design files
- `\App`: Python code to run GUI to visualise and log the signals
- `\ESP32`: C++ firmware, developed using PlatformIO, to run on ESP32 and interface with the PCB to acquire signals
  - `\ESP32\test`: Unit tests for firmware
- `\Classification`: Jupyter notebook to generate MLP model weights

## User Guide

### Visualising and Logging Data

Visualising and logging data using this device is simple to set up with little installation. The steps are detailed below.

1. Clone the project repository by running the following commands in the terminal of the desired file location.

   `git clone https://github.com/abbyfinka/FYP-Knitted-EMG`

   `cd FYP-Knitted-EMG/App`

3. Run the Python script `EMGApp.py`. A GUI will open showing live data. An indicator light on the device will turn on when the device has successfully connected.

4. Logs can be found at `FYP-Knitted-EMG/App/Logs/[TIMESTAMP]_emg_log.txt`

If data is not visible, check the terminal. If there are any issues connecting to the ESP32, first check that there are no other Bluetooth devices connected to your device. If the issue persists check that the ESP32 is turned on and in range, this can be verified using an app such as nRF Connect.

To use the device for acquiring signals other than sEMG, the Python GUI can be configured by minor changes to the source code. `logging` can be set to `False` to disable logging received data to a text file, setting `filtering` to `True` will enable real time band pass filtering of the signal, with cutoff frequencies determined by `fc_lowpass` and `fc_highpass`. Additionally, adjusting `ylim_time`, `ylim_freq` and `PRINT_INTERVAL` can adjust the initial axis of the time and frequency plots.

### Updating Embedded Classification Model

The AIfES can easily be updated with new weights, the steps required are detailed below.

1. Clone the project repository by running the following commands in the terminal of the desired file location.

   `git clone https://github.com/abbyfinka/FYP-Knitted-EMG`

   `cd FYP-Knitted-EMG/Classification`

3. Setup EMGPromptApp, accessible [here](https://github.com/LilOz/EMGPromptApp), developed by Ayman Osman, to collect training data. User instructions can be found in the repository README.

    Update line 318 of `main.py`, to account for the increased sampling rate. 
    
    Corrected line: `time_step = 0.001`

4. Collect data with this application. Run the developed Python GUI instead of the OpenBCI software.

5. Upload training data and run `MLP-Training-for-ESP32.ipynb`. The last cell will print a string that can be copied into `FYP-Knitted-EMG/ESP32/lib/Weights/weights.h`

6. Upload the new firmware to the ESP32. Classification output is available over BLE in the GUI.

### Using the Device

Firmware for signal acquisition is already uploaded to the device, for any changes, connect the programming PCB to upload new code.

1. Connect LiPo battery to switch on device, red LED will turn on to indicate power.
   
2. Click the RESET button to start the code. Blue LED should flash on and off. If the blue LED stays on, the ADS1198 has not been initialised correctly, press RESET again.
   
3. Connect to the device as described above, blue LED will turn on to indicate BLE connection

