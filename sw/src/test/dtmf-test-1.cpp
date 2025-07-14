/*
A unit test of the DTMF detector
*/
#include <iostream>
#include <cmath>
#include <cassert>

#include "kc1fsz-tools/DTMFDetector.h"

using namespace std;
using namespace kc1fsz;

static const float PI = 3.1415926;

int main(int,const char**) {

    // 160 samples is 20ms at 8 kHz

    unsigned int fs = 8000;

    DTMFDetector detector(fs);

    int16_t silence[160];
    for (unsigned int i = 0; i < 160; i++) 
        silence[i] = 0;

    // Simple test
    {
        // Make a test signal (20ms)
        int16_t test1[160];
        for (unsigned int i = 0; i < 160; i++) {
            // Two tones
            float a = 0.45 * cos((float)i * 2.0 * PI * 1209.0 / (float)fs);
            float b = 0.5 * cos((float)i * 2.0 * PI * 770.0 / (float)fs);
            float t = a + b;
            // Scale up to a int16
            test1[i] = 32767.0 * t;
        }

        // Make a bogus signal (20ms)
        int16_t testBad[160];
        for (unsigned int i = 0; i < 160; i++) {
            // Two tones
            float a = 0.45 * cos((float)i * 2.0 * PI * 1209.0 / (float)fs);
            // Invalid freq
            float b = 0.5 * cos((float)i * 2.0 * PI * 790.0 / (float)fs);
            float t = a + b;
            // Scale up to a int16
            testBad[i] = 32767.0 * t;
        }

        // 40ms tone w/ 40ms silence (valid DSC)
        detector.play(test1, 160);
        detector.play(test1, 160);
        detector.play(silence, 160);
        detector.play(silence, 160);

        assert(detector.isAvailable() == 1);
        assert(detector.pullResult() == '4');
        assert(detector.isAvailable() == 0);

        // 40ms bad tone w/ 40ms silence (not valid DSC)
        detector.play(testBad, 160);
        detector.play(testBad, 160);
        detector.play(silence, 160);
        detector.play(silence, 160);

        assert(detector.isAvailable() == 0);

        // 20ms tone w/ 40ms silence (not valid DSC)
        detector.play(test1, 160);
        detector.play(silence, 160);
        detector.play(silence, 160);

        assert(detector.isAvailable() == 0);

        // 60ms tone w/ 40ms silence (valid DSC)
        detector.play(test1, 160);
        detector.play(test1, 160);
        detector.play(test1, 160);
        detector.play(silence, 160);
        detector.play(silence, 160);

        assert(detector.isAvailable() == 1);
        assert(detector.pullResult() == '4');
        assert(detector.isAvailable() == 0);

        // 80ms tone w/ 20ms drop then 40ms silence (valid DSC)
        detector.play(test1, 160);
        detector.play(test1, 160);
        detector.play(silence, 160);
        detector.play(test1, 160);
        detector.play(silence, 160);
        detector.play(silence, 160);

        assert(detector.isAvailable() == 1);
        assert(detector.pullResult() == '4');
        assert(detector.isAvailable() == 0);

        // 80ms tone w/ 20ms garbage then 40ms silence (valid DSC)
        detector.play(test1, 160);
        detector.play(test1, 160);
        detector.play(testBad, 160);
        detector.play(test1, 160);
        detector.play(silence, 160);
        detector.play(silence, 160);

        assert(detector.isAvailable() == 1);
        assert(detector.pullResult() == '4');
        assert(detector.isAvailable() == 0);

    }




    return 0;
}

