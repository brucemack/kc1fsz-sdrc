import numpy as np
import matplotlib.pyplot as plt
import math
from scipy.signal import lfilter, firwin

fs = 8000
pi = 3.1415926

def filt1(signal0, ft1):

    g_w = 2.0 * pi * ft1 / fs
    g_cw = math.cos(g_w)
    g_sw = math.sin(g_w)
    g_c = 2.0 * g_cw
    g_z1 = 0
    g_z2 = 0

    for i in range(0, len(signal0)):
        g_z0 = signal0[i] + g_c * g_z1 - g_z2
        g_z2 = g_z1
        g_z1 = g_z0

    g_i = g_cw * g_z1 - g_z2
    g_q = g_sw * g_z1
    g_mag = math.sqrt(g_i * g_i + g_q * g_q)
    return g_mag

# Simulate a signal being filtered
samples = 512
t = np.linspace(0, samples, samples, endpoint=False)

ft0 = 123
a0 = 1.0
omega0 = 2.0 * pi * ft0 / fs
signal0 = a0 * np.sin(omega0 * t)

mean1 = 0  # Center of the distribution
# NOTE: If the mean of the noise is zero, RMS noise and 
# standard deviation are equivalent! 
stddev1 = 0.5 * 0.707 # Standard deviation (spread of the noise)#
# Generate Gaussian white noise
signal1 = np.random.normal(loc=mean1, scale=stddev1, size=len(t))
signal2 = signal0 + signal1
#signal2 = signal0

# Try a range of frequencies to see which match the best.
result = []
f0 = np.linspace(50, 200, 300, endpoint=False)
for f in f0:
    # Here the DFT bucket gets divided by samples/2
    re = filt1(signal2, f) / (samples / 2)
    #print(f, 20 * math.log10(re))
    #result.append(20 * math.log10(re))
    result.append(20 * math.log10(re))

fig, ax = plt.subplots()
ax.plot(f0, result)
#ax.plot(t, signal1, color='red')
ax.set_title('CTCSS Filter Response (ft=' + str(ft0) + ')')
ax.set_xlabel('Frequency (Hz)')
ax.set_ylabel('Magnitude')
ax.grid(True)
plt.show()
