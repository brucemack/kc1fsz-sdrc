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
from scipy import signal
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

fs = 8000
# Center frequency in Hz
f0 = 123
# -3dB bandwidth in Hz
fbw = 110
# Quality factor. Dimensionless parameter that characterizes notch filter -3 dB bandwidth 
# bw relative to its center frequency, Q = w0/bw
q = f0 / fbw
# Synthesize the filter (2nd order)
b1, a1 = signal.iirnotch(f0, q, fs)
# Convert to second-order sections format
sos1 = signal.tf2sos(b1, a1)

# Synthesize the filter (2nd order)
b2, a2 = signal.iirnotch(f0, q, fs * 2)
# Convert to second-order sections format
sos2 = signal.tf2sos(b2, a2)

sos3 = np.vstack((sos1, sos2))

f1 = 190

# Frequency response plot
#freq, h = signal.freqz(b1, a1, fs=fs)
freq, h = signal.freqz_sos(sos3, fs=fs)

fig, ax = plt.subplots(2, 1, figsize=(8, 6))
ax[0].plot(freq, 20*np.log10(abs(h)), color='blue')
ax[0].set_title("PL Notch Frequency Response")
ax[0].set_ylabel("Amplitude [dB]", color='blue')
ax[0].set_ylim([-60, 6])
ax[0].set_xscale('log')
ax[0].axvline(f0, color='red', linestyle='--', label='Notch Frequency ' + str(f0) + " Hz, q=" + 
    "{:.1f}".format(q))
ax[0].axvline(f1, color='green', linestyle='--', label='-3dB Point ' + str(f1) + " Hz")
ax[0].legend()
ax[0].grid(True)

ax[1].plot(freq, np.unwrap(np.angle(h))*180/np.pi, color='green')
ax[1].set_ylabel("Phase [deg]", color='green')
ax[1].set_xlabel("Frequency [Hz]")
ax[1].set_xscale('log')
#ax[1].set_yticks([-90, -60, -30, 0, 30, 60, 90])
#ax[1].set_ylim([-90, 90])
ax[1].grid(True)

plt.show()

# Sanity check: generate a sample signal of one second
t = np.linspace(0, fs, fs, endpoint=False)
ft = f1
omega = 2 * 3.1415926 * ft / fs
s = np.sin(omega * t) + 0.000 * np.random.randn(len(t))

# Apply the filter
filtered_s = signal.sosfilt(sos3, s)

# Pull of the leading part of the series
t = t[250:]
s = s[250:]
filtered_s = filtered_s[250:]

# Compute the RMS powers
print("RMS of original", rms(s))
print("RMS of filtered", rms(filtered_s))
print("dB", 10 * math.log10(rms(filtered_s)/rms(s)))

# Plot resulting signal
fig, ax = plt.subplots()
ax.plot(t, filtered_s)
ax.set_title('Filtered Signal (ft=' + str(ft) + ')')
ax.set_xlabel('Tone Freq (Hz)')
ax.set_ylabel('Magnitude')

ax.grid(True)
plt.show()

