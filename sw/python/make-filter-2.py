"""
Generation of FIR LPF for removing low-frequency CTCSS tones from audio.

Copyright (C) Bruce MacKinnon, 2025
Software Defined Repeater Controller Project

Need to add this to the path:
export PYTHONPATH=../../../firpm-py
"""
import numpy as np
import matplotlib.pyplot as plt
import math
from scipy.signal import lfilter, firwin
import firpm 

def rms(signal):
    signal_array = np.array(signal)
    rms_value = np.sqrt(np.mean(signal_array**2))
    return rms_value

def fir_freq_response(coefficients, sampling_rate):
    # Calculate the frequency response using the DFT (FFT)
    frequency_response = np.fft.fft(coefficients)    
    # Number of coefficients
    num_coeffs = len(coefficients)
    # Generate the corresponding frequency values
    frequencies = np.fft.fftfreq(num_coeffs, 1 / sampling_rate)
    return frequencies, frequency_response

# The maximum taps allowed at the moment
taps = 127
fs = 8000
print("HPF for CTCSS elimination below 250")
# Making this transition band a bit wider eliminated a lot of ripple
wc0 = 100 / fs
wc1 = 225 / fs
# Sliding up
#wc0 = 175 / fs
#wc1 = 250 / fs
h, _ = firpm.design(taps, 1, 2, [ 0.00, wc0, wc1, 0.5 ], [ 0.00, 1.0 ], [ 1.0, 1.0 ] )
h_reverse = list(h)
h_reverse.reverse()
print("Reverse Impulse Response", h_reverse)

sampling_rate = 1  # Hz

# Calculate the frequency response
frequencies, frequency_response = fir_freq_response(h, fs)

# Plot the magnitude and phase responses
db_data = 20 * np.log10(np.abs(frequency_response[:len(frequencies)//2]))
fig, ax = plt.subplots()
ax.plot(frequencies[:len(frequencies)//2], db_data)
ax.set_xscale('log') 
ax.set_title('CTCSS Filter Magnitude Response')
ax.set_xlabel('Frequency (Hz)')
ax.set_ylabel('Magnitude (dB)')
ax.grid(True)
plt.show()

# Sanity check: generate a sample signal of one second
t = np.linspace(0, fs, fs, endpoint=False)
ft = 200
omega = 2 * 3.1415926 * ft / fs
signal = np.sin(omega * t)

# Apply the FIR filter
filtered_signal = lfilter(h, [1.0], signal)

# Compute the RMS powers
print("RMS of original", rms(signal))
print("RMS of filtered", rms(filtered_signal))
print("dB", 10 * math.log10(rms(filtered_signal)/rms(signal)))






# Plot resulting signal
fig, ax = plt.subplots()
ax.plot(t[127:], filtered_signal[127:])
ax.set_title('Filtered Signal (ft=' + str(ft) + ')')
ax.set_xlabel('Tone Freq (Hz)')
ax.set_ylabel('Magnitude')
ax.grid(True)
plt.show()
