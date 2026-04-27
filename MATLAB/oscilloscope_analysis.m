data = readtable("C:\Users\abbyf\OneDrive - Imperial College London\Documents\Year 4\FYP\1cope_0.csv");
y = table2array(data(3:end, 2));
x = table2array(data(3:end, 1));

Fs = 1 / (x(2) - x(1));
L = length(y);

Y = fft(y);
P2 = abs(Y / L);
P1 = P2(1:floor(L/2)+1);
P1(2:end-1) = 2 * P1(2:end-1);
f = Fs * (0:floor(L/2)) / L;

figure;
plot(f, P1, 'LineWidth', 1.5);
title('Single-Sided Amplitude Spectrum of y(t)');
xlabel('Frequency (Hz)');
ylabel('Magnitude |P1(f)|');
grid on;