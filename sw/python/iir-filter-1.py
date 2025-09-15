import math
import numpy as np
from scipy import signal
import matplotlib.pyplot as plt

#fs = 1280
#cutoff_freq = 150
fs = 150
cutoff_freq = 30
#fs = 32000
#cutoff_freq = 2100

Ts = 1 / fs
nyquist_freq = 0.5 * fs
normalized_cutoff = cutoff_freq / nyquist_freq

# Using 'sos' output for stability
#sos = signal.butter(2, normalized_cutoff, btype='low', output='sos')
#sos = signal.butter(1, normalized_cutoff, btype='high', output='sos')

# Return format (b, a) where the b coefficients are multiplied by the X(z) samples
# and the a coefficients are multiplied by the Y(z) samples.
# 
# For example: 
#
# a0 Y(z) = b0 X(z) + b1 X(z) Z-1 + a1 Y(z) Z-1

#sos = signal.butter(1, normalized_cutoff, btype='high', output='sos')
sos = signal.butter(1, normalized_cutoff, btype='low', output='sos')
print(sos)
"""
# Do this specific first-order HPF case manually
wc = 2 * np.pi * cutoff_freq
wc_warped = math.tan(wc * Ts / 2.0)
b0_2 = 1 / (1 + wc_warped)
b1_2 = -b0_2 
a0_2 = 1
a1_2 = -b0_2 * (1 - wc_warped)
sos = []
sos.append([b0_2, b1_2, 0.0, a0_2, a1_2, 0.0 ])
"""
print("b0", sos[0][0])
print("b1", sos[0][1])
print("a0", sos[0][3])
print("a1", sos[0][4])
b0 = sos[0][0]
b1 = sos[0][1]
a0 = sos[0][3]
a1 = sos[0][4]

# Compute frequency response for SOS filter
w, h = signal.freqz_sos(sos, worN=1000, fs=fs) 
# worN is number of points, fs for Hz output

 # Plotting the magnitude response in dB
plt.figure()
plt.semilogx(w, 20 * np.log10(np.maximum(abs(h), 1e-6))) # np.maximum for avoiding log(0)
plt.title('IIR Filter Frequency Response')
plt.xlabel('Frequency [Hz]')
plt.ylabel('Amplitude [dB]')
plt.grid(which='both', axis='both')
plt.axvline(cutoff_freq, color='green', linestyle='--', label='Cutoff Frequency')
plt.legend()
plt.show()

# Sanity check: generate a sample signal of one second
t = np.linspace(0, fs, fs, endpoint=False)
ft = cutoff_freq
omega = 2 * 3.1415926 * ft / fs
s = np.sin(omega * t)

# Apply the IIR filter

# The manual way
result = []
prev_x = 0
prev_y = 0
for i in range(0, s.shape[0]):
    x = s[i]
    y = b0 * x + b1 * prev_x - a1 * prev_y
    prev_x = x
    prev_y = y 
    result.append(y)
filtered_s = np.array(result)    

# The SCIPY way
#filtered_s = signal.sosfilt(sos, s)

# Plot resulting signal
fig, ax = plt.subplots()
ax.plot(t[0:255], filtered_s[0:255])
ax.set_title('Filtered Signal (ft=' + str(ft) + ')')
ax.set_xlabel('Tone Freq (Hz)')
ax.set_ylabel('Magnitude')
ax.grid(True)
plt.show()

