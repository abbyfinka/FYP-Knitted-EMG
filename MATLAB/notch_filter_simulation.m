
b0 = 0.98478425;
b1 = -1.87317095;
b2 = 0.98478425;
a0 = 1.0;
a1 = -1.87317095;
a2 = 0.96956849;

b = [b0, b1, b2];
a = [a0, a1, a2];

fs = 1000;
t = 0:1/fs:1-1/fs;
clean_signal = 0.5*randn(size(t));
x = sin(2*pi*50*t) + clean_signal;

y = filter(b, a, x);

figure;
subplot(3,1,1);
plot(t, x); title('Original Signal'); xlabel('Time (s)'); ylabel('Amplitude');
subplot(3,1,2);
plot(t, y); title('Filtered Signal'); xlabel('Time (s)'); ylabel('Amplitude');
subplot(3,1,3);
plot(t, clean_signal); title('Clean Signal'); xlabel('Time (s)'); ylabel('Amplitude');