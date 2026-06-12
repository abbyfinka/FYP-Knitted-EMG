%% load EMG data

clear; clc; 
load("DSP_AtRest_R2.mat");
data_dsp = channel_data(:,3);

load("NoFiltering_AtRest_R2.mat")
data_nofiltering = channel_data(:,3);

load("AFE_AtRest_L1.mat")
data_ad8232 = channel_data(:,3);

%%

load("Electrode-1-1.mat")
data_close_electrode = channel_data(:,3);
load("Electrode-1-2.mat")
data_close_electrode2 = channel_data(:,3);
load("Electrode-2-1.mat")
data_far_electrode = channel_data(:,3);
load("Electrode-2-2.mat")
data_far_electrode2 = channel_data(:,3);

%%

load("3xfist_espfiltering.mat")
data_fist_espfilt = channel_data(:,3);

%% plot data

% load("Data/Dataset1.mat");
load("Data/3xfist_espfiltering.mat")

% start_index = 6450;
% end_index = 9000;

start_index = 1;
end_index = 32632;


figure('Position', [100, 100, 800, 600]);
set(gca, 'FontSize', 14);
hold on;
grid on;
title({"EMG Signal of Fist Position after Hand at Rest,", "Raw Signal, x12 Gain on ADS1198 "})
xlabel("Time / s");
% xlim([229.5, 232]);
ylabel("Measured Voltage / mV")
plot(esp_timestamp(start_index:end_index) ./ 1000000, channel_data(start_index:end_index,3));
% exportgraphics(gcf, 'Figures/Fist-Raw-No-Filters.pdf', 'ContentType', 'vector');


b0 = 0.98478425;
b1 = -1.87317095;
b2 = 0.98478425;
a0 = 1.00000000;
a1 = -1.87317095;
a2 = 0.96956849;

b = [b0, b1, b2];
a = [a0, a1, a2];

x_nofiltering = channel_data(start_index:end_index,3);

x_bandpass_nofiltering = bandpass(x_nofiltering,[20 450],sampling_rate);
x_notch_nofiltering = filtfilt(b, a, x_bandpass_nofiltering);

figure('Position', [100, 100, 800, 600]);
set(gca, 'FontSize', 14);
hold on;
grid on;
title({"EMG Signal of Fist Position after Hand at Rest, Signal", "with Offline Notch and Bandpass Filters, x12 Gain on ADS1198"})
xlabel("Time / s");
%xlim([229.5, 232]);
%ylim([-100, 100]);
ylabel("Measured Voltage / mV")
plot(esp_timestamp(start_index:end_index) ./ 1000000, x_notch_nofiltering);
% exportgraphics(gcf, 'Figures/Fist-Raw-w-Filters.pdf', 'ContentType', 'vector');

%%

Y = fft(channel_data(:,3));
L_dsp = length(channel_data(:,3));
P2_dsp = abs(Y / L_dsp);
P1_dsp = P2_dsp(1:floor(L_dsp/2)+1);
P1_dsp(2:end-1) = 2 * P1_dsp(2:end-1);
f = sampling_rate * (0:floor(L_dsp/2)) / L_dsp;

figure;
plot(f, P1_dsp, 'LineWidth', 1.5);
title('Single-Sided Amplitude Spectrum of y(t)');
xlabel('Frequency (Hz)');
ylabel('Magnitude |P1(f)|');
grid on;

%%

Y_dsp = fft(data_dsp);
L_dsp = length(data_dsp);
P2_dsp = abs(Y_dsp / L_dsp);
P1_dsp = P2_dsp(1:floor(L_dsp/2)+1);
P1_dsp(2:end-1) = 2 * P1_dsp(2:end-1);
f_dsp = sampling_rate * (0:floor(L_dsp/2)) / L_dsp;

Y_nf = fft(data_nofiltering);
L_nf = length(data_nofiltering);
P2_nf = abs(Y_nf / L_nf);
P1_nf = P2_nf(1:floor(L_nf/2)+1);
P1_nf(2:end-1) = 2 * P1_nf(2:end-1);
f_nf = sampling_rate * (0:floor(L_nf/2)) / L_nf;

Y_ad = fft(data_ad8232);
L_ad = length(data_ad8232);
P2_ad = abs(Y_ad / L_ad);
P1_ad = P2_ad(1:floor(L_ad/2)+1);
P1_ad(2:end-1) = 2 * P1_ad(2:end-1);
f_ad = sampling_rate * (0:floor(L_ad/2)) / L_ad;

% figure;
% hold on;
% plot(f_ad, P1_ad, 'LineWidth', 1.5);
% plot(f_dsp, P1_dsp, 'LineWidth', 1.5);
% plot(f_nf, P1_nf, 'LineWidth', 1.5);
% legend("AD8232", "RTDSP", "No Filtering");
% title('Single-Sided Amplitude Spectrum of y(t)');
% xlabel('Frequency (Hz)');
% ylabel('Magnitude |P1(f)|');
% grid on;

figure;
hold on;
plot(f_ad, P1_ad, 'LineWidth', 1.5);
title('Frequency Spectrum of sEMG Output with AD8232');
xlabel('Frequency (Hz)');
ylabel('Magnitude |P1(f)|');
grid on;
exportgraphics(gcf, 'Figures/FFT-AD8232.pdf', 'ContentType', 'vector');

figure;
hold on;
plot(f_dsp, P1_dsp, 'LineWidth', 1.5);
title('Frequency Spectrum of sEMG Output with Digital Signal Processing on ESP32');
xlabel('Frequency (Hz)');
ylabel('Magnitude |P1(f)|');
grid on;
exportgraphics(gcf, 'Figures/FFT-DSP.pdf', 'ContentType', 'vector');

figure;
hold on;
plot(f_nf, P1_nf, 'LineWidth', 1.5);
title('Frequency Spectrum of sEMG Output with No Filtering');
xlabel('Frequency (Hz)');
ylabel('Magnitude |P1(f)|');
grid on;
exportgraphics(gcf, 'Figures/FFT-NF.pdf', 'ContentType', 'vector');


%% 

Y_far = fft(data_far_electrode);
L_far = length(data_far_electrode);
P2_far = abs(Y_far / L_far);
P1_far = P2_far(1:floor(L_far/2)+1);
P1_far(2:end-1) = 2 * P1_far(2:end-1);
f_far = sampling_rate * (0:floor(L_far/2)) / L_far;

Y_far2 = fft(data_far_electrode2);
L_far2 = length(data_far_electrode);
P2_far2 = abs(Y_far2 / L_far2);
P1_far2 = P2_far2(1:floor(L_far2/2)+1);
P1_far2(2:end-1) = 2 * P1_far2(2:end-1);
f_far2 = sampling_rate * (0:floor(L_far2/2)) / L_far2;

Y_close = fft(data_close_electrode);
L_close = length(data_close_electrode);
P2_close = abs(Y_close / L_close);
P1_close = P2_close(1:floor(L_close/2)+1);
P1_close(2:end-1) = 2 * P1_close(2:end-1);
f_close = sampling_rate * (0:floor(L_close/2)) / L_close;

Y_close2 = fft(data_close_electrode2);
L_close2 = length(data_close_electrode);
P2_close2 = abs(Y_close2 / L_close2);
P1_close2 = P2_close2(1:floor(L_close2/2)+1);
P1_close2(2:end-1) = 2 * P1_close2(2:end-1);
f_close2 = sampling_rate * (0:floor(L_close2/2)) / L_close2;

% figure;
% hold on;
% plot(f_ad, P1_ad, 'LineWidth', 1.5);
% plot(f_dsp, P1_dsp, 'LineWidth', 1.5);
% plot(f_nf, P1_nf, 'LineWidth', 1.5);
% legend("AD8232", "RTDSP", "No Filtering");
% title('Single-Sided Amplitude Spectrum of y(t)');
% xlabel('Frequency (Hz)');
% ylabel('Magnitude |P1(f)|');
% grid on;

figure;
hold on;
plot(f_far2, P1_far2, 'LineWidth', 1.5);
plot(f_far, P1_far, 'LineWidth', 1.5);
plot(f_close2, P1_close2, 'LineWidth', 1.5);
plot(f_close, P1_close, 'LineWidth', 1.5);
legend("Far", "Far", "Close", "Close");
title('Frequency Spectrum of sEMG Output from 2 electrodes');
xlabel('Frequency (Hz)');
ylabel('Magnitude |P1(f)|');
grid on;
exportgraphics(gcf, 'Figures/FFT-distance-comparison.pdf', 'ContentType', 'vector');

%%

Y = fft(data_fist_espfilt);
L = length(data_fist_espfilt);
P2 = abs(Y / L);
P1 = P2(1:floor(L/2)+1);
P1(2:end-1) = 2 * P1(2:end-1);
f = sampling_rate * (0:floor(L/2)) / L;

figure;
hold on;
plot(f, P1, 'LineWidth', 1.5);
plot(f_close, P1_close, 'LineWidth', 1.5);
legend("Far", "Far", "Close", "Close");
title('Frequency Spectrum of sEMG Output from 2 electrodes');
xlabel('Frequency (Hz)');
ylabel('Magnitude |P1(f)|');
grid on;


%% 

load("C:\Imperial\FYP-Knitted-EMG\MATLAB\Data\AD8232-ch3-esp32notchfilter.mat")
data = channel_data(:,3);

% time domain

figure;
hold on;
plot(esp_timestamp ./ 1000000, data, "LineWidth", 1.5);
title("Plot of AD8232 Output in Time Domain with Knitted Electrodes");
xlabel("Time / s");
ylabel("Amplitude / mV");

% frequency domain

Y = fft(data);
L = length(data);
P2 = abs(Y / L);
P1 = P2(1:floor(L/2)+1);
P1(2:end-1) = 2 * P1(2:end-1);
f = sampling_rate * (0:floor(L/2)) / L;

figure;
hold on;
plot(f, P1, 'LineWidth', 1.5);
title('Frequency Spectrum of AD8232 Output with Knitted Electrodes');
xlabel('Frequency (Hz)');
ylabel('Magnitude');
grid on;

%% 

load("C:\Imperial\FYP-Knitted-EMG\MATLAB\Data\NoFiltering-StandardElectrodes-Data1.mat")
data = channel_data(:,3);

% time domain

figure;
hold on;
plot(esp_timestamp ./ 1000000, data, "LineWidth", 1.5);
title("Plot of EMG Output in Time Domain with Standard Electrodes");
xlabel("Time / s");
ylabel("Amplitude / mV");

% frequency domain

Y = fft(data);
L = length(data);
P2 = abs(Y / L);
P1 = P2(1:floor(L/2)+1);
P1(2:end-1) = 2 * P1(2:end-1);
f = sampling_rate * (0:floor(L/2)) / L;

figure;
hold on;
plot(f, P1, 'LineWidth', 1.5);
title('Frequency Spectrum of EMG Output with Standard Electrodes');
xlabel('Frequency (Hz)');
ylabel('Magnitude');
grid on;
ylim([0,1]);
xlim([0, 1000]);