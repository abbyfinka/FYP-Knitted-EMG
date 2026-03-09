'''
App to display EMG signals obtained by the device, data is received over serial
Based on similar application: https://github.com/LilOz/emg-realtime-classification
BLE interface based on: https://github.com/HamzaYslmn/ESP32_BT_Windows_Python/tree/main 
'''
import asyncio
import threading
from bleak import BleakClient, BleakScanner
from collections import deque
import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec
from matplotlib.animation import FuncAnimation

# ======= Global parameters =======

SERVICE_UUID = "cde33313-b7aa-4b32-b29f-9043b1d8e042"
CHARACTERISTIC_UUID = "89fea506-0482-4895-b474-843229dae557"
TARGET_DEVICE_NAME = "EMG-Logger"
PRINT_INTERVAL = 0.512

fs = 1000
data_buffer_len = fs * 3
time_step = 1 / fs

logging = 0 # Set to 1 to enable logging to text file, 0 to disable

# ======= Recieving BLE =======

async def process_data(queue):
    while True:
        # print('processing data...')
        data = await queue.get()
        
        # processing data
        try: 
            raw_data = data.decode('utf-8').strip()
            split_data = raw_data.split(',')
            #print(split_data)
            for n in range(0,7):
                # append new values
                channel_data[n].append(float(split_data[n]) * -1)
                # print(f"Channel {n+1}: {split_data[n]}")
        except Exception as e:
            print("Error decoding data " + str(e))

        queue.task_done()


async def ble_receive():
    print('Starting BLE receive...')
    # asyncio queue to hold incoming data

    data_queue = asyncio.Queue()

    # The callback function just throws data in the bucket
    def notification_handler(sender, data):
        data_queue.put_nowait(data)

    # Start the background processing task when data is put in queue
    process_task = asyncio.create_task(process_data(data_queue))

    print(f"Scanning for device named '{TARGET_DEVICE_NAME}'...")
    
    device = await BleakScanner.find_device_by_filter(
        lambda d, ad: d.name and d.name.lower() == TARGET_DEVICE_NAME.lower(),
        timeout=10.0 # 10s timeout
    )
    
    if not device:
        print(f"Error: Could not find '{TARGET_DEVICE_NAME}'")
        return

    print(f"Found '{device.name}' at address: {device.address}")

    print("Connecting...")
    async with BleakClient(device.address) as client:
        print("Connected!")
        
        await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
        
        print("Streaming data. Press Ctrl+C to stop.")
        try:
            while True:
                await asyncio.sleep(1)
        except asyncio.CancelledError:
            pass
        finally:
            await client.stop_notify(CHARACTERISTIC_UUID)
            process_task.cancel()


def run_ble_process():
    asyncio.run(ble_receive())

# ======= Global data buffer =======

channel_data = [deque(maxlen=data_buffer_len) for _ in range(8)]

# ======= Start BLE thread =======

print('Starting BLE thread...')
threading.Thread(target=run_ble_process, daemon=True).start()

# ======= Plotting =======

fig = plt.figure(figsize=(12, 10))
gs = GridSpec(2, 4, figure=fig)

emg_axes = []
lines = []

print("Setting up plots...")
for i in range(8):
    ax = fig.add_subplot(gs[i // 4, i % 4])
    line, = ax.plot([], [], label=f"Channel {i+1}", linewidth=0.5)
    lines.append(line)
    ax.set_ylim(0, 600)
    ax.set_xlim(0, data_buffer_len)
    ax.set_ylabel(f"Ch {i+1}")
    ax.set_title(f"Channel {i+1}")
    emg_axes.append(ax)

emg_axes[-1].set_xlabel("Samples")
plt.tight_layout()

def animate(frame):
    for idx, line in enumerate(lines):
        y = list(channel_data[idx])
        x = list(range(len(y)))
        line.set_data(x, y)

    # Return all three line objects so FuncAnimation knows what to redraw
    return lines

ani = FuncAnimation(fig, animate, interval=50, blit=True)
plt.tight_layout()
plt.show()



    
