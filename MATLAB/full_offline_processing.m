%% load EMG data

clear; clc; 
load("dataset1.mat");

%% plot data

figure(1);
plot(esp_timestamp ./ 1000, channel_data(:,3));


%% offline filtering

for i = [1:num_channels]
    bandpass(x,[100 200],fs)
end

