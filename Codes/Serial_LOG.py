import serial
import time
import csv

# Configure the serial port and baud rate
SERIAL_PORT = '/dev/cu.usbserial-FTB6SPL3'  # Change this to your Arduino's port
BAUD_RATE = 115200
CSV_FILE = 'timer_log.csv'
n_meas = 150

# Initialize serial connection
ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
time.sleep(2)  # Wait for the serial connection to initialize

# Function to send commands to Arduino
def send_command(command):
    ser.write(command.encode())
    time.sleep(0.1)  # Small delay to ensure the command is sent

# Function to log stopped times to CSV
def log_stopped_time(index, t1, t2):
    with open(CSV_FILE, mode='a', newline='') as file:
        writer = csv.writer(file)
        writer.writerow([index, t1, t2])

# Main loop
try:
    index = 0
    t1 = t2 = 0
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()
            print(line)  # Print all serial messages

            if line.startswith("Stopped Timer: "):
                # Extract the display number and time from the message
                parts = line.split(" : ")
                display_number = int(parts[0])
                minutes = int(parts[1])
                seconds = int(parts[2])
                hundreds = int(parts[3])

                # Calculate the total elapsed time in seconds
                total_time = minutes * 60 + seconds + hundreds / 100.0

                # Log the stopped time
                if display_number == 0:
                    t1 = total_time
                elif display_number == 1:
                    t2 = total_time

                # Log the time to CSV
                log_stopped_time(index, t1, t2)
                index += 1
                
                print("Starting measurement: " + index)
                if index <= n_meas :
                    send_command("timing[0] = true;")
                    send_command("timing[1] = true;")
                    
except KeyboardInterrupt:
    print("Exiting...")
finally:
    ser.close()