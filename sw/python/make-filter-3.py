"""
Generation of FIR HPF for measuring high-frequency noise content of audio.

Copyright (C) Bruce MacKinnon, 2025
Software Defined Repeater Controller Project
"""
import numpy as np
import matplotlib.pyplot as plt
import math
import firpm 
from scipy.signal import lfilter, firwin

def fir_freq_response(coefficients, sampling_rate):
    # Calculate the frequency response using the DFT (FFT)
    frequency_response = np.fft.fft(coefficients)    
    # Number of coefficients
    num_coeffs = len(coefficients)
    # Generate the corresponding frequency values
    frequencies = np.fft.fftfreq(num_coeffs, 1/sampling_rate)
    return frequencies, frequency_response

fs = 32000

# Create the high-pass filter used to measure noise
hp_taps = 21
hp_wc0 = 5000 / fs
hp_wc1 = 7000 / fs
hp_h, _ = firpm.design(hp_taps, 1, 2, [ 0.00, hp_wc0, hp_wc1, 0.5 ], [ 0.0, 1.0 ], [ 1.0, 1.0 ] )
hp_h_reverse = list(hp_h)
hp_h_reverse.reverse()
#print("Noise Filter", hp_h)
print("Reverse Noise Filter", hp_h_reverse)

# Simulate a signal being filtered
t = np.linspace(0, int(fs / 100), int(fs / 100), endpoint=False)

# This is a 2kHz tone at the 32000 sampling rate
ft0 = 2000
a0 = 7
omega0 = 2.0 * 3.1415926 * ft0 / fs
signal0 = a0 * np.sin(omega0 * t)

mean1 = 0  # Center of the distribution
# NOTE: If the mean of the noise is zero, RMS noise and 
# standard deviation are equivalent! 
stddev1 = 0.707 # Standard deviation (spread of the noise)#
# Generate Gaussian white noise
signal1 = np.random.normal(loc=mean1, scale=stddev1, size=len(t))
# Combine the signal and noise
signal2 = signal0 + signal1

# Compute the RMS of the signal
rms0 = np.sqrt(np.mean(signal0**2))
rms1 = np.sqrt(np.mean(signal1**2))

# Apply the HP filter
signal3 = lfilter(hp_h, [1.0], signal2)
rms3 = np.sqrt(np.mean(signal3**2))

