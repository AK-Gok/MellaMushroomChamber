import serial
import csv
import time
from datetime import datetime

# Serial port configuration
SERIAL_PORT = '/dev/cu.usbmodem2101' # Replace with your actual port
BAUD_RATE = 115200 # Set to match the device's baud rate

# CSV file configuration
path = '/Users/milton01/Documents/mella_logs/'
CSV_FILE = f"/Users/milton01/Documents/mella_logs/serial_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

# Open the serial port
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

# Open CSV file for writing
with open(CSV_FILE, mode='w', newline='') as file:
    csv_writer = csv.writer(file)
    csv_writer.writerow(['Timestamp', 'Data'])

    print(f"Writing to {CSV_FILE} every 5 seconds... Press Ctrl+C to stop.")

    try:
        while True:
            # Read line from serial port
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                
                # Write data to CSV
                timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                csv_writer.writerow([timestamp, line])
                print(f"{timestamp}, {line}")
                
            # Sleep for 5 seconds
            time.sleep(5)

    except KeyboardInterrupt:
        print("Stopped by user.")
    finally:
        ser.close()
        print("Serial port closed.")
