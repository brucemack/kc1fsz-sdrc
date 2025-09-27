import numpy as np
import matplotlib.pyplot as plt
import math
from scipy.signal import lfilter, firwin

fs = 32000
ft = 2000
N = 1024
T = 1 / fs

# Sanity check: generate a sample signal of one second
t = np.linspace(0, 1024, 1024, endpoint=False)
omega = 2 * 3.1415926 * ft / fs
y = np.sin(omega * t)

# DFT
# Compute the FFT
yf = np.fft.fft(y)
xf = np.fft.fftfreq(N, T)[:N//2] # Frequencies corresponding to the FFT output

# Plot the magnitude spectrum (positive frequencies)
plt.plot(xf, 2.0/N * np.abs(yf[0:N//2]))
plt.grid()
plt.xlabel("Frequency [Hz]")
plt.ylabel("Amplitude")
plt.title("FFT of a Signal")
plt.show()