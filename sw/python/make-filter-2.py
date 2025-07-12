import numpy as np
import matplotlib.pyplot as plt
import math
from scipy.signal import lfilter, firwin
import firpm 

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

# The maximum taps allowed at the moment
taps = 127
fs = 8000
print("HPF for CTCSS elimination below 250")
wc0 = 200 / fs
wc1 = 375 / fs

h, dev = firpm.design(taps, 1, 2, [ 0.00, wc0, wc1, 0.5 ], [ 0.00, 1.0 ], [ 1.0, 1.0 ] )
#h, dev = firpm.design(taps, 1, 3, [ 0.00, wc0, wc1, wc2, wc3, 0.5 ], [ 0.00, 1.0, 0.00 ], [ 1.0, 1.0, 1.0 ] )

print("Impulse Response", h)
#print("Dev dB", 20 * math.log10(dev))

sampling_rate = 1  # Hz

# Calculate the frequency response
frequencies, frequency_response = fir_freq_response(h, fs)

# Plot the magnitude and phase responses
db_data = 20 * np.log10(np.abs(frequency_response[:len(frequencies)//2]))

fig, ax = plt.subplots()
ax.plot(frequencies[:len(frequencies)//2], db_data)
ax.set_xscale('log') 
#ax.set_yscale('log') 
ax.set_title('CTCSS Filter Magnitude Response')
ax.set_xlabel('Frequency (Hz)')
ax.set_ylabel('Magnitude (dB)')
ax.grid(True)
plt.show()

# Generate a sample signal of one second
t = np.linspace(0, fs, fs, endpoint=False)
ft = 134
omega = 2 * 3.1415926 * ft / fs
signal = np.sin(omega * t)

# Apply the FIR filter
filtered_signal = lfilter(h, [1.0], signal)

fig, ax = plt.subplots()
ax.plot(t[127:], filtered_signal[127:])
ax.set_title('Filtered Signal (ft=' + str(ft) + ')')
ax.set_xlabel('Tone Freq (Hz)')
ax.set_ylabel('Magnitude')
ax.grid(True)
plt.show()
