from datetime import datetime
from collections import deque
import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec
from matplotlib.animation import FuncAnimation
from pathlib import Path
import numpy as np
from scipy import signal
from itertools import islice
import csv

# -------------------- Display settings -------------------- #
PRINT_INTERVAL = 0.512
sampling_frequency = 1000
data_buffer_len = sampling_frequency * 2
time_step = 1 / sampling_frequency
ylim_time = 10
ylim_freq = 200
sample_index = 1
global_index = 0

gestures = ("Rest", 
            "Fist", 
            "Flexion",
            "Extension",
            "Radial",
            "Ulnar")

channel_data = [deque(maxlen=data_buffer_len) for _ in range(8)] # Data buffer

# -------------------- Get Data -------------------- #

ch_data = [[] for _ in range(8)]
data_path = "Logs/2026-06-15_14-12-20_emg_log.txt"
with open(data_path, 'r') as f:
    reader = csv.DictReader(islice(f, 4, None))
    
    for row in reader:
        for i in range(0, 8):
            raw_data = row[f' EXG Channel {i}']
            float_list = [float(x) for x in raw_data.split(',') if x.strip()]
            ch_data[i].append(float_list)

# -------------------- Plotting -------------------- #

fig = plt.figure(figsize=(12, 10))
gs = GridSpec(8, 2, figure=fig, width_ratios=[1,1])

time_plots = []
freq_plots = []

time_axes = []
freq_axes = []

print("Setting up plots...")
for i in range(8):
    # time domain plots
    ax_time = fig.add_subplot(gs[i, 0])
    time_plot, = ax_time.plot([], [], label=f"Channel {i+1}", linewidth=0.5)
    time_plots.append(time_plot)
    ax_time.set_ylim(-ylim_time, ylim_time)
    ax_time.set_xlim(0, data_buffer_len * time_step)
    ax_time.set_ylabel(f"Ch {i+1} / mV")
    time_axes.append(ax_time)

    # frequency domain plots
    ax_freq = fig.add_subplot(gs[i, 1])
    freq_plot, = ax_freq.plot([], [], label=f"Channel {i+1}", linewidth=0.5)
    freq_plots.append(freq_plot)
    ax_freq.set_xlim(0, sampling_frequency/2)
    ax_freq.set_ylim(0, ylim_freq)
    freq_axes.append(ax_freq)

fig.suptitle("- - -%", fontsize=16, fontweight="bold")
time_axes[-1].set_xlabel("Time / s")
freq_axes[-1].set_xlabel("Frequency / Hz")
plt.tight_layout()


def animate(frame):
    global global_index
    
    if global_index + data_buffer_len >= len(ch_data[0]):
        return []

    for ch_index, line in enumerate(time_plots):
        y = [item for sublist in ch_data[ch_index][global_index:global_index+data_buffer_len] for item in sublist]
        x = [s * time_step for s in range(len(y))]
        line.set_data(x, y)

    for ch_index, line in enumerate(freq_plots):
        y = np.array([item for sublist in ch_data[ch_index][global_index:global_index+data_buffer_len] for item in sublist], dtype=float)
        
        if len(y) > 1:
            freqs = np.fft.rfftfreq(len(y), d=time_step)
            fft_values = np.abs(np.fft.rfft(y))
            line.set_data(freqs, fft_values)

    global_index += 50
    
    return time_plots + freq_plots

ani = FuncAnimation(fig, animate, interval=50, blit=True)
plt.tight_layout()
plt.show()
