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

fs = 32000

# Create the high-pass filter used to measure noise
hp_taps = 21
hp_wc0 = 5000 / fs
hp_wc1 = 7000 / fs
hp_h, _ = firpm.design(hp_taps, 1, 2, [ 0.00, hp_wc0, hp_wc1, 0.5 ], [ 0.0, 1.0 ], [ 1.0, 1.0 ] )
print("Noise Filter", hp_h)

# Create the low-pass filter used to decimate 
df_taps = 21
df_wc0 = 3000 / fs
df_wc1 = 5000 / fs
df_h, _ = firpm.design(hp_taps, 1, 2, [ 0.00, hp_wc0, hp_wc1, 0.5 ], [ 1.0, 0.0 ], [ 1.0, 1.0 ] )

# Create the band-pass filter used to strip CTCSS
bp_taps = 127
bp_fs = fs / 4
bp_wc0 = 200 / bp_fs
bp_wc1 = 375 / bp_fs
bp_wc2 = 3000 / bp_fs
bp_wc3 = 3175 / bp_fs
bp_h, _ = firpm.design(bp_taps, 1, 3, [ 0.00, bp_wc0, bp_wc1, bp_wc2, bp_wc3, 0.5 ], [ 0.00, 1.0, 0.00 ], [ 1.0, 1.0, 1.0 ] )

"""
# Calculate the frequency response
frequencies, frequency_response = fir_freq_response(h, fs)
db_data = 20 * np.log10(np.abs(frequency_response[:len(frequencies)//2]))
fig, ax = plt.subplots()
ax.plot(frequencies[:len(frequencies)//2], db_data)
ax.set_xscale('log') 
#ax.set_yscale('log') 
ax.set_title('Magnitude Response')
ax.set_xlabel('Frequency (Hz)')
ax.set_ylabel('Magnitude (dB)')
ax.grid(True)
plt.show()
"""

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

# Combine the signals
signal2 = signal0 + signal1

# Compute the RMS of the signal
rms0 = np.sqrt(np.mean(signal0**2))
rms1 = np.sqrt(np.mean(signal1**2))

# Apply the FIR filter
#fsignal0 = lfilter(h, [1.0], signal0)
#fsignal1 = lfilter(h, [1.0], signal1)
#frms1 = np.sqrt(np.mean(fsignal1**2))

# Apply the HP filter
signal3 = lfilter(hp_h, [1.0], signal2)
# Apply the decimation filter
signal4 = lfilter(df_h, [1.0], signal2)
# Do the decimation down to 8kHz
signal5 = signal4[::4]
# Apply the band pass filter to the 8kHz
signal6 = lfilter(bp_h, [1.0], signal5)

rms3 = np.sqrt(np.mean(signal3**2))
rms6 = np.sqrt(np.mean(signal6**2))

print("S/N",20 * math.log10(rms6/rms3))

"""
fig, ax = plt.subplots()
#ax.plot(t, fsignal0)
ax.plot(t, fsignal1)
ax.set_title('Filtered Signal')
ax.set_xlabel('Time')
ax.set_ylabel('Magnitude')
ax.grid(True)
plt.show()
"""
