import serial
import time

port = '/dev/cu.usbmodem2101'  # Update this if needed
baud_rate = 115200

output_file_path = 'mella_log_output.txt'  # Change to .csv if needed

try:
    ser = serial.Serial(port, baud_rate, timeout=1)
    print(f"Connected to {port} at {baud_rate} bps")

    with open(output_file_path, 'a') as file:
        while True:
            if ser.in_waiting > 0:
                data = ser.readline().decode('utf-8', errors='replace').strip()
                timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
                log_line = f"{timestamp}, {data}\n"                                 # For .csv,  log_line = f'"{timestamp}","{data}"\n'

                #print(f"Received: {data}"). ## don't want to print
                file.write(log_line)
                file.flush()  # Force write to disk

            #time.sleep(0.1). ## removed because missing lines

except serial.SerialException as e:
    print(f"Serial error: {e}")
except KeyboardInterrupt:
    print("Stopped by user")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Serial port closed")