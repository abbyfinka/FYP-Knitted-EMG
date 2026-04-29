%% load EMG data

clear; clc; 
load("dataset1.mat");

%% plot data

figure(1);
plot(esp_timestamp ./ 1000, channel_data(:,3));

%% create 50 Hz notch filter

notch_freq = 50;
Wo = notch_freq/(sampling_rate/2);  
Q = 20; % ratio of notch freq to bandwidth
BW = Wo/Q;

notchFilter = designNotchPeakIIR(FilterOrder=2, CenterFrequency=Wo, Response='notch', SystemObject=true);
dfv = dsp.DynamicFilterVisualizer(NormalizedFrequency=true,YLimits=[-60, 20]);
dfv(notchFilter)

%% offline filtering


for i = 1:num_channels
    x_bandpass = bandpass(x,[20 500],sampling_rate);
    x_notch = filter(b, a, x_bandpass);
end

