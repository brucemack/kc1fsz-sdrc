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

// Function to generate a block of white noise samples
void generateWhiteNoiseQ31(q31_t* out, unsigned numSamples, double amplitude) {
    // Obtain a random number from hardware
    std::random_device rd;
    // Seed the generator
    std::mt19937 gen(rd()); 
    std::uniform_real_distribution<> distrib(-amplitude, amplitude);
    for (unsigned int i = 0; i < numSamples; ++i)
        out[i] = distrib(gen) * 2147483648.0f;
}

/**
 * @brief The file is assumed to be in 16-bit PCM format, so a conversion
 * to 32-bit PCM (q31_t) is needed as well.
 */
unsigned loadFromFile(const char* fn, int32_t* target, unsigned target_max) {
    ifstream is(fn);
    unsigned i = 0;
    float scale = 32767.0;
    std::string line;
    while (getline(is, line))
        if (i < target_max)
            target[i++] = (stof(line) / scale) * 2147483648.0;
    return i;
}

int main(int argc, const char** argv) {

    AudioCore core0(0, 2), core1(1, 2);

    core0.setCtcssDecodeFreq(123);
    core0.setCtcssEncodeFreq(123);
    core0.setCtcssEncodeLevel(-26);
    core0.setCtcssEncodeEnabled(true);
    core0.setRxDelayMs(100);
    core0.setToneEnabled(false);
    core0.setToneFreq(1000);
    core0.setToneLevel(-10);
    core0.setCrossGainLinear(0, 1.0);
    core0.setCrossGainLinear(1, 0.0);

    core1.setCtcssDecodeFreq(88.5);
    core1.setCrossGainLinear(0, 0.5);
    core1.setCrossGainLinear(1, 0.5);

    // TEMP
    //core0.setDiagToneFreq(2000);
    //core0.setDiagToneLevel(-3);
    //core0.setDiagToneEnabled(true);

    ofstream os("/tmp/clip-3b.txt");
    bool noiseSquelchEnabled = true;
    enum SquelchState { OPEN, CLOSED, TAIL }
        squelchState = SquelchState::CLOSED;
    float lastSnr = 0;
    unsigned tailCount = 0;

    unsigned outerCount = 1;
    unsigned sweepTone = 50;
    unsigned sweepToneIncrement = 50;

    // Outer test loop
    for (unsigned outer = 0; outer < outerCount; outer++) {

        const unsigned test_in_max = AudioCore::FS_ADC * 7;
        const unsigned test_blocks = test_in_max / AudioCore::BLOCK_SIZE_ADC;
        int32_t test_in_0[test_in_max];
        int32_t test_in_1[test_in_max];
        for (unsigned i = 0; i < test_in_max; i++) {
            test_in_0[i] = 0;
            test_in_1[i] = 0;
        }

        // Fill in the test audio
        float ft = 300;
        //float ft = sweepTone;
        //generateWhiteNoiseQ31(test_in_0, test_in_max, 1.0);
        //make_real_tone_q31(test_in_0, test_in_max, AudioCore::FS_ADC, ft, 0.98); 
        loadFromFile("../tests/clip-3.txt", test_in_0, test_in_max);

        int32_t* adc_in_0 = test_in_0;
        int32_t* adc_in_1 = test_in_1;

        float cross_out_0[AudioCore::BLOCK_SIZE];
        float cross_out_1[AudioCore::BLOCK_SIZE];
        const float* cross_ins[2] = { cross_out_0, cross_out_1 };

        int32_t dac_out_0[AudioCore::BLOCK_SIZE_ADC];
        int32_t dac_out_1[AudioCore::BLOCK_SIZE_ADC];

        float last_o_0 = 0;

        for (unsigned block = 0; block < test_blocks; block++) {

            core0.cycleRx(adc_in_0, cross_out_0);
            core1.cycleRx(adc_in_1, cross_out_1);
            core0.cycleTx(cross_ins, dac_out_0);
            core1.cycleTx(cross_ins, dac_out_1);

            adc_in_0 += AudioCore::BLOCK_SIZE_ADC;
            adc_in_1 += AudioCore::BLOCK_SIZE_ADC;

            // Write out a block of audio at 32K
            for (unsigned i = 0; i < AudioCore::BLOCK_SIZE_ADC; i++) {
                if (!noiseSquelchEnabled ||
                    squelchState != SquelchState::CLOSED) {
                    os << (dac_out_0[i] >> 16) << endl; 
                } else {
                    os << 0 << endl;
                }
            }
                        
            float n_0 = AudioCore::vrmsToDbv(core0.getNoiseRms());
            float s_0 = AudioCore::vrmsToDbv(core0.getSignalRms());
            float snr = AudioCore::vrmsToDbv(core0.getSignalRms() / core0.getNoiseRms());
            double pl_0 = AudioCore::vrmsToDbv(core0.getCtcssDecodeRms());
            double o_0 = AudioCore::vrmsToDbv(core0.getOutRms());
            last_o_0 = o_0;
   
            // Calculate the noise squelch with hysteresis
            bool threshold = snr > 10 && pl_0 > -31;

            char state;
            if (squelchState == SquelchState::CLOSED)
                state = 'C';
            else
                state = 'O';

            cout << block << " " << snr << " " << state << " " << pl_0 << " " << o_0 << endl;
            //cout << block << " " << s_0 << " " << n_0 << " " << pl_0 << endl;

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

        // Output
        //cout << sweepTone << "\t" << last_o_0 << endl;

        sweepTone += sweepToneIncrement;
    }

    os.close();

    return 0;
}
