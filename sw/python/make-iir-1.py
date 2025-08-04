from scipy import signal
import numpy as np
import matplotlib.pyplot as plt

# Filter parameters
fs = 100
fc = 20
order = 2
cutoff_freq_norm = fc / fs  # Normalized cutoff frequency (Nyquist = 1)
filter_type = 'highpass'
filter_design = 'cheby1'

# rp=1 means 1dB ripple
b, a = signal.iirfilter(order, cutoff_freq_norm, rp=1, btype=filter_type, ftype=filter_design)
print("b", b)
print("a", a)

# Generate a sample signal
t = np.linspace(0, 1, fs, endpoint=False)
fa = 20
sig = np.sin(2 * np.pi * fa * t)

# Apply the filter
filtered_sig = signal.lfilter(b, a, sig)

w, h = signal.freqz(b, a)
plt.plot(w / np.pi, 20 * np.log10(abs(h)))
plt.title('Filter Frequency Response')
plt.xlabel('Normalized Frequency (Nyquist = 1)')
plt.ylabel('Gain (dB)')
plt.grid()
plt.show()
