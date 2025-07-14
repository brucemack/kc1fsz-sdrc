# Converts from .TXT to .WAV format
# Bruce MacKinnon 9-Nov-2023
#
import scipy.io.wavfile as wavfile
import numpy as np

samplerate = 32000; 
in_fn = "../tests/clip-3b.txt"
out_fn = "/mnt/c/tmp/clip-3b.wav"

with open(in_fn, "r") as txt_file:
    data = np.array([int(line) for line in txt_file])
wavfile.write(out_fn, samplerate, data.astype(np.int16))
