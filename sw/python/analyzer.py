import serial
import time
import matplotlib.pyplot as plt
import math

try:
    ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=None) 
    print(f"Serial port {ser.name} opened successfully.")
except:
    print("Failed")
    quit()

figure, ax = plt.subplots()
hl, = plt.plot([], [], '-')
ax.grid()
ax.set_autoscale_on(True)
ax.set_xlabel("Frequency [Hz]")
ax.set_ylabel("Amplitude")
ax.set_ylim(-25,3)
ax.set_xlim(0,8000)
#ax.title("FFT of a Signal")
plt.ioff()

c = 0

while True:
    received_data = ser.readline().decode('utf-8').strip()
    print(f"Received: {received_data}")
    if received_data.startswith("SWEEP "):
        tokens = received_data.split(" ")
        start_hz = float(tokens[1])
        step_hz = float(tokens[2])
        freq = start_hz
        freqs = []
        data = []
        for token in tokens[3:]:
            freqs.append(freq)
            data.append(20.0 * math.log(float(token)))
            freq = freq + step_hz
        #print("Freq", freqs)
        #print("Data", data)
        hl.set_xdata(freqs)
        hl.set_ydata(data)
        #ax.relim()
        ax.autoscale_view()
        plt.pause(0.05)
        figure.canvas.draw()
        figure.canvas.flush_events()

ser.close()



