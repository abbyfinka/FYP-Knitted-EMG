import asyncio
from bleak import BleakScanner

async def scan_for_devices():
    print("Scanning for Bluetooth devices for 5 seconds...")
    
    # Discover devices nearby
    devices = await BleakScanner.discover(timeout=5.0)
    
    if not devices:
        print("No devices found.")
        return

    print("\nFound these devices:")
    print("-" * 40)
    for device in devices:
        # device.name is often None if the device doesn't broadcast a name
        name = device.name if device.name else "Unknown Device"
        
        # device.address is the MAC address (or UUID on macOS)
        print(f"Name: {name}")
        print(f"MAC Address: {device.address}")
        print(f"RSSI (Signal Strength): {device.rssi} dBm")
        print("-" * 40)

# Run the async loop
asyncio.run(scan_for_devices())