import numpy as np
import matplotlib.pyplot as plt
import math

# Implementation of the Goertzel Algorithm. This
# returns the magnitude and angle of the selected complex
# frequency.
def goertzel_polar(s, ft, fs):

    g_w = 2.0 * np.pi * ft / fs
    g_cw = math.cos(g_w)
    g_sw = math.sin(g_w)
    g_c = 2.0 * g_cw
    g_z1 = 0
    g_z2 = 0

    for v in s:
        g_z0 = v + g_c * g_z1 - g_z2
        g_z2 = g_z1
        g_z1 = g_z0

    g_r = g_cw * g_z1 - g_z2
    g_i = g_sw * g_z1
    # NOTE: The expensive square root and arctan are here:
    return np.abs(complex(g_r, g_i)), np.angle(complex(g_r, g_i))

N = 512
ft = 125
fs = 8000
a = 1.0
t = np.linspace(0, N, N, endpoint=False)
omega = 2.0 * np.pi * ft / fs
y = a * np.cos(omega * t)

# IMPORTANT: The result of Goertzel is consistent with
# what would normally happen in a DFT bin - the magnitude
# needs to be scaled by 2.0 / N to match the Vp magnitude
# of the original signal.
g = goertzel_polar(y, ft, fs)
print("Mag", g[0] * 2.0 / N, "Angle", int(math.degrees(g[1])))

