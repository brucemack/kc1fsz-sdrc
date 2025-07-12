# Low-pass filter used for interpolation from 
# 8K to 32K.
import numpy as np
import matplotlib.pyplot as plt
import math
import firpm 
from scipy.signal import lfilter, firwin

def fir_freq_response(coefficients, sampling_rate):
    """
    Calculates the frequency response of an FIR filter.

    Args:
        coefficients: A list or NumPy array of FIR filter coefficients.
        sampling_rate: The sampling rate of the signal.

    Returns:
        A tuple containing:
            - frequencies: A NumPy array of frequencies (Hz).
            - frequency_response: A NumPy array of complex frequency response values.
    """
    # Calculate the frequency response using the DFT (FFT)
    frequency_response = np.fft.fft(coefficients)
    
    # Number of coefficients
    num_coeffs = len(coefficients)
    
    # Generate the corresponding frequency values
    frequencies = np.fft.fftfreq(num_coeffs, 1/sampling_rate)
    
    return frequencies, frequency_response

taps = 127
fs = 32000
wc0 = 2000 / fs
wc1 = 2600 / fs
h, dev = firpm.design(taps, 1, 2, [ 0.00, wc0, wc1, 0.5 ], [ 1.0, 0.0 ], [ 1.0, 1.0 ] )

print("Impulse Response", h)

sampling_rate = 1  # Hz

# Calculate the frequency response
frequencies, frequency_response = fir_freq_response(h, fs)

# Plot the magnitude response
db_data = 20 * np.log10(np.abs(frequency_response[:len(frequencies)//2]))
fig, ax = plt.subplots()
ax.plot(frequencies[:len(frequencies)//2], db_data)
ax.set_xscale('log') 
#ax.set_yscale('log') 
ax.set_title('Interpolation LPF Filter Magnitude Response')
ax.set_xlabel('Frequency (Hz)')
ax.set_ylabel('Magnitude (dB)')
ax.grid(True)
plt.show()

# Generate a sample signal of one second
t = np.linspace(0, fs, fs, endpoint=False)
ft = 2500
omega = 2 * 3.1415926 * ft / fs
signal = np.sin(omega * t)

# Apply the FIR filter
filtered_signal = lfilter(h, [1.0], signal)

fig, ax = plt.subplots()
ax.plot(t[41:1000], filtered_signal[41:1000])
ax.set_title('Filtered Signal (ft=' + str(ft) + ')')
ax.set_xlabel('Tone Freq (Hz)')
ax.set_ylabel('Magnitude')
ax.grid(True)
plt.show()
