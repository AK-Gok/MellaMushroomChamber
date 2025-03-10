import serial
import time

port = '/dev/cu.usbmodem2101' # Replace with your actual port
baud_rate = 115200 # Set to match the device's baud rate

try:
    ser = serial.Serial(port, baud_rate)
    print(f"Connected to {port} at {baud_rate} bps")

    while True:
        if ser.in_waiting > 0:
            data = ser.readline().decode('utf-8').strip() # Read line, decode, and remove extra whitespace
            print(f"Received: {data}")
        time.sleep(0.1) # Small delay to prevent excessive CPU usage

except serial.SerialException as e:
    print(f"Error: {e}")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Serial port closed")