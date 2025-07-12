# Converts from .WAV to .TXT format
# Bruce MacKinnon 9-Nov-2023
#
import scipy.io.wavfile as wavfile
import numpy as np

in_fn = "../kc1fsz-rc1/tests/clip-3.wav"
out_fn = "../kc1fsz-rc1/tests/clip-3.txt"

samplerate, data = wavfile.read(in_fn)

print("Sample Rate (Hz)   ", samplerate)
print("Sample Count       ", data.size)
print("Duration (Seconds) ", data.size / samplerate)
print("Min                ", data.min())
print("Max                ", data.max())
print("Mean               ", data.mean())

# Create a text file
count = 0
with open(out_fn, "w") as txt_file:
    #txt_file.write("// Audio data in 16-bit signed PCM  values\n")
    #txt_file.write("const short audio_id[] = {\n")
    for x in np.nditer(data):
        #txt_file.write(str(x) + ",\n")
        txt_file.write(str(x) + "\n")
        #count = count + 1
    #txt_file.write("};\n")
    #txt_file.write("const unsigned int audio_id_length = " + str(count) +";\n")








