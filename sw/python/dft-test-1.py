import numpy as np
import matplotlib.pyplot as plt
import math
from scipy.signal import lfilter, firwin

N = 1024
fs = 8000
#ft = (fs / 2) - (fs / N) 
#ft = (fs / 2)
#ft = (fs / N)
ft = 123
T = 1 / fs
vp = 1.0
dc_offset = 1.0

# Sanity check: generate a sample signal of one second
t = np.linspace(0, 1024, 1024, endpoint=False)
omega = 2 * 3.1415926 * ft / fs
y = vp * np.sin(omega * t) + dc_offset

# DFT
# Compute the FFT
yf = np.fft.fft(y)
xf = np.fft.fftfreq(N, T)[:N//2]

print("0", 1.0/N * np.abs(yf[0]))
print("1", 1.0/N * np.abs(yf[1]))
print("N-1", 1.0/N * np.abs(yf[1023]))
print("N/2", 1.0/N * np.abs(yf[512]))
print("N/2-1", 1.0/N * np.abs(yf[511]))
print("N/2+1", 1.0/N * np.abs(yf[513]))

# Plot the magnitude spectrum (positive frequencies)
#plt.plot(xf, 2.0/N * np.abs(yf[0:N//2]))
plt.plot(xf, np.angle(yf[0:N//2]))
plt.grid()
plt.xlabel("Frequency [Hz]")
plt.ylabel("Amplitude")
plt.title("FFT of a Signal")
plt.show()