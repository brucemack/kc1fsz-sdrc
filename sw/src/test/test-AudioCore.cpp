#include <iostream>
#include <fstream>
#include <random>
#include <string>

// Radlib
#include "util/dsp_util.h"

#include "AudioCore.h"

using namespace std;
using namespace kc1fsz;
using namespace radlib;

static float db(float l) {
    return 20.0 * log10(l);
}

// Function to generate a block of white noise samples
void generateWhiteNoise(unsigned int numSamples, double amplitude, float* out) {
    // Obtain a random number from hardware
    std::random_device rd;
    // Seed the generator
    std::mt19937 gen(rd()); 
    std::uniform_real_distribution<> distrib(-amplitude, amplitude);
    for (unsigned int i = 0; i < numSamples; ++i)
        out[i] = distrib(gen);
}

unsigned loadFromFile(const char* fn, float* target, 
    unsigned target_max) {
    ifstream is(fn);
    unsigned i = 0;
    float scale = 32767.0;
    std::string line;
    while (getline(is, line))
        if (i < target_max)
            target[i++] = stof(line) / scale;
    return i;
}

int main(int argc, const char** argv) {

    AudioCore core0(0), core1(1);
    core0.setCtcssDecodeFreq(123);
    core0.setCtcssEncodeFreq(123);
    core0.setCtcssEncodeLevel(-26);
    core0.setCtcssEncodeEnabled(true);
    core0.setDelayMs(100);
    core1.setCtcssDecodeFreq(88.5);

    ofstream os("../tests/clip-3b.txt");
    float ft = 123;
    bool noiseSquelchEnabled = true;
    enum SquelchState { OPEN, CLOSED, TAIL }
        squelchState = SquelchState::CLOSED;
    float lastSnr = 0;
    unsigned tailCount = 0;

    // Outer test loop
    for (unsigned outer = 0; outer < 1; outer++) {

        const unsigned test_in_max = AudioCore::FS_ADC * 7;
        const unsigned test_blocks = test_in_max / AudioCore::BLOCK_SIZE_ADC;
        float test_in_0[test_in_max];
        float test_in_1[test_in_max];
        for (unsigned i = 0; i < test_in_max; i++) {
            test_in_0[i] = 0;
            test_in_1[i] = 0;
        }

        // Fill in the test audio
        //generateWhiteNoise(test_in_len / 2, 1.0, test_in_0);
        //make_real_tone_f32(test_in_0, test_in_max, AudioCore::FS_ADC, ft, 0.98); 
        unsigned test_in_0_len = loadFromFile("../tests/clip-3.txt", test_in_0, test_in_max);

        //ft = 88.5;
        //make_real_tone_f32(test_in_1, test_in_max, AudioCore::FS_ADC, ft); 

        float* adc_in_0 = test_in_0;
        float* adc_in_1 = test_in_1;

        float cross_out_0[AudioCore::BLOCK_SIZE];
        float cross_out_1[AudioCore::BLOCK_SIZE];
        const float* cross_in[2] = { cross_out_0, cross_out_1 };

        float dac_out_0[AudioCore::BLOCK_SIZE_ADC];
        float dac_out_1[AudioCore::BLOCK_SIZE_ADC];

        for (unsigned block = 0; block < test_blocks; block++) {

            // Cycle 0
            core0.cycle0(adc_in_0, cross_out_0);
            core1.cycle0(adc_in_1, cross_out_1);

            // Cycle 1
            core0.cycle1(cross_in, dac_out_0);
            core1.cycle1(cross_in, dac_out_1);

            adc_in_0 += AudioCore::BLOCK_SIZE_ADC;
            adc_in_1 += AudioCore::BLOCK_SIZE_ADC;

            // Write out a block of audio at 8K
            //for (unsigned i = 0; i < AudioCore::BLOCK_SIZE; i++) {
            //    os << (int)(cross_out_0[i] * 32767.0) << endl; 
            //}

            // Write out a block of audio at 32K
            for (unsigned i = 0; i < AudioCore::BLOCK_SIZE_ADC; i++) {
                if (!noiseSquelchEnabled ||
                    squelchState != SquelchState::CLOSED) {
                    os << (int)(dac_out_0[i] * 32767.0) << endl; 
                    //os <<                dac_out_0[i] << endl;
                } else {
                    os << 0 << endl;
                }
            }
                        
            float n_0 = db(core0.getNoiseRms());
            float s_0 = db(core0.getSignalRms());
            float snr = core0.getSnr();

            double plDb = db(core0.getCtcssDecodeMag());
    
            // Calculate the noise squelch with hysteresis
            bool threshold = snr > 10 && plDb > -28;

            char state;
            if (squelchState == SquelchState::CLOSED)
                state = 'C';
            else
                state = 'O';

            cout << block << " " << snr << " " << state << " " << plDb << endl;

            if (squelchState == SquelchState::CLOSED) {
                // Look for unsquelch
                if (threshold) {
                    squelchState = SquelchState::OPEN;
                    // On the unsquelch we need to arm 
                    // the delay counter to avoid revealing 
                    // any historical noise
                    core0.resetDelay();
                } 
            }
            else if (squelchState == SquelchState::TAIL) {
                // In this case the tail is interrupted 
                // and we go back into normal unsquelched mode.
                if (threshold) {
                    squelchState = SquelchState::OPEN;
                }
                // Here the tail has ended 
                else if (tailCount == 0) {
                    squelchState = SquelchState::CLOSED;
                }
                // Count down waiting for the tail to end
                else {
                    tailCount--;
                }
            }
            else if (squelchState == SquelchState::OPEN) {
                if (threshold) {
                }
                // Here is where we start the tail
                else {
                    squelchState = SquelchState::TAIL;
                    // If we dropping from a high number then 
                    // squelch immediately
                    //if (lastSnr > 20) {
                    //    tailCount = 18;
                    //} 
                    //else {
                        tailCount = 8;
                    //}
                }
            }
            lastSnr = snr;
        }
    }

    os.close();

    return 0;
}
