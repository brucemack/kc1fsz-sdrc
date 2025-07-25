/*
A unit test of the DTMF detector
*/
#include <iostream>
#include <cmath>
#include <cassert>

#include <arm_math.h>
#include "DTMFDetector2.h"

using namespace std;
using namespace kc1fsz;

//static const float PI = 3.1415926;

int main(int,const char**) {

    const unsigned FS = 8000;
    const unsigned N = 64;

    DTMFDetector2 detector;
    float threshold = pow(10.0, (-30.0/20.0));
    printf("Vrms threshold %f\n", threshold);
    detector.setSignalThreshold(-55);

    float silence[N];
    for (unsigned int i = 0; i < N; i++) 
        silence[i] = 0;

    float vrms_target = pow(10.0, (-20.0/20.0));
    printf("Vrms target %f\n", vrms_target);
    float vp_target = vrms_target * (1.0/0.707);
    printf("Vp target %f\n", vp_target);

    cout << "----- Test 1 ------" << endl;
    {
        // Make a test signal (48ms)
        float test1[N * 6];
        for (unsigned int i = 0; i < N * 6; i++) {
            // Two valid tones
            float a = vp_target * cos((float)i * 2.0 * PI * 1209.0 / (float)FS);
            float b = vp_target * cos((float)i * 2.0 * PI * 770.0 / (float)FS);
            float t = a + b;
            test1[i] = t;
        }

        // 40ms silence then 40ms tone (valid DSC)
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(test1);
        detector.processBlock(test1 + N);
        detector.processBlock(test1 + N * 2);
        detector.processBlock(test1 + N * 3);
        detector.processBlock(test1 + N * 4);
        detector.processBlock(test1 + N * 5);
        //detector.processBlock(silence);
        //detector.processBlock(silence);

        assert(detector.isDetectionPending());
        assert(detector.popDetection() == '4');
        assert(!detector.isDetectionPending());
    }

    {
        cout << "----- Test 1a (Twist. col > row) ------" << endl;
        // Make a test signal (48ms)
        float test1[N * 6];
        for (unsigned int i = 0; i < N * 6; i++) {
            // Two valid tones
            // COLUMN 0
            float a = vp_target * 10 * cos((float)i * 2.0 * PI * 1209.0 / (float)FS);
            // ROW 1
            float b = vp_target * cos((float)i * 2.0 * PI * 770.0 / (float)FS);
            float t = a + b;
            test1[i] = t;
        }

        // 40ms tone w/ 40ms silence (valid DSC)
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(test1);
        detector.processBlock(test1 + N);
        detector.processBlock(test1 + N * 2);
        detector.processBlock(test1 + N * 3);
        detector.processBlock(test1 + N * 4);
        detector.processBlock(test1 + N * 5);
        detector.processBlock(silence);
        detector.processBlock(silence);

        assert(!detector.isDetectionPending());
    }

    {
        cout << "----- Test 1b (Acceptable twist: col > row) ------" << endl;
        // Make a test signal (48ms)
        float test1[N * 6];
        for (unsigned int i = 0; i < N * 6; i++) {
            // Two valid tones
            // COLUMN 0
            float a = vp_target * 1.25 * cos((float)i * 2.0 * PI * 1209.0 / (float)FS);
            // ROW 1
            float b = vp_target * cos((float)i * 2.0 * PI * 770.0 / (float)FS);
            float t = a + b;
            // Scale up to a int32 fixed 
            test1[i] = t;
        }

        // 40ms tone w/ 40ms silence (valid DSC)
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(test1);
        detector.processBlock(test1 + N);
        detector.processBlock(test1 + N * 2);
        detector.processBlock(test1 + N * 3);
        detector.processBlock(test1 + N * 4);
        detector.processBlock(test1 + N * 5);
        detector.processBlock(silence);
        detector.processBlock(silence);

        assert(detector.isDetectionPending());
        assert(detector.popDetection() == '4');
        assert(!detector.isDetectionPending());
    }

    cout << "----- Test 1c (Two Symbols) ------" << endl;
    {
        // Make a test signal (48ms)
        float test1[N * 6];
        for (unsigned int i = 0; i < N * 6; i++) {
            // Two valid tones
            float a = vp_target * cos((float)i * 2.0 * PI * 1209.0 / (float)FS);
            float b = vp_target * cos((float)i * 2.0 * PI * 770.0 / (float)FS);
            float t = a + b;
            // Scale up to a int32 fixed 
            test1[i] = t;
        }

        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(test1);
        detector.processBlock(test1 + N);
        detector.processBlock(test1 + N * 2);
        detector.processBlock(test1 + N * 3);
        detector.processBlock(test1 + N * 4);
        detector.processBlock(test1 + N * 5);

        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);

        assert(detector.isDetectionPending());
        assert(detector.popDetection() == '4');
        assert(!detector.isDetectionPending());

        for (unsigned int i = 0; i < N * 6; i++) {
            // Two valid tones
            float a = vp_target * cos((float)i * 2.0 * PI * 1209.0 / (float)FS);
            float b = vp_target * cos((float)i * 2.0 * PI * 941.0 / (float)FS);
            float t = a + b;
            // Scale up to a int32 fixed 
            test1[i] = t;
        }

        // NOTE: Quite a bit of silence required here
        detector.processBlock(silence);
        detector.processBlock(test1);
        detector.processBlock(test1 + N);
        detector.processBlock(test1 + N * 2);
        detector.processBlock(test1 + N * 3);
        detector.processBlock(test1 + N * 4);
        detector.processBlock(test1 + N * 5);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);

        assert(detector.isDetectionPending());
        assert(detector.popDetection() == '*');
        assert(!detector.isDetectionPending());
    }

    {
        cout << "----- Test 2 ------" << endl;
        // Make a bogus signal (48ms)
        float testBad[N * 6];
        for (unsigned int i = 0; i < N * 6; i++) {
            // Two tones
            float a = 0.45 * cos((float)i * 2.0 * PI * 1209.0 / (float)FS);
            // Invalid freq by 30 hz, which should be enough to break the 58Hz
            // resolution requirement.
            float b = 0.5 * cos((float)i * 2.0 * PI * (770 - 30) / (float)FS);
            float t = a + b;
            testBad[i] = t;
        }

        // 40ms bad tone w/ 40ms silence (not valid DSC)
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(testBad);
        detector.processBlock(testBad + N);
        detector.processBlock(testBad + N * 2);
        detector.processBlock(testBad + N * 3);
        detector.processBlock(testBad + N * 4);
        detector.processBlock(testBad + N * 5);
        detector.processBlock(silence);
        detector.processBlock(silence);

        assert(!detector.isDetectionPending());
    }

    {
        cout << "----- Test 3 ------" << endl;
        // Make a test signal (48ms)
        float test1[N * 6];
        for (unsigned int i = 0; i < N * 6; i++) {
            // Two valid tones
            float a = vp_target * cos((float)i * 2.0 * PI * 1209.0 / (float)FS);
            float b = vp_target * cos((float)i * 2.0 * PI * 770.0 / (float)FS);
            float t = a + b;
            // Scale up to a int32 fixed 
            test1[i] = t;
        }

        // 20ms tone w/ 40ms silence (not valid DSC)
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(test1);
        detector.processBlock(test1 + N);
        detector.processBlock(test1 + 2 * N);
        detector.processBlock(silence);
        detector.processBlock(silence);

        assert(!detector.isDetectionPending());
    }

    {
        cout << "----- Test 4 ------" << endl;
        // Make a test signal (48ms)
        float test1[N * 6];
        for (unsigned int i = 0; i < N * 6; i++) {
            // Two valid tones
            float a = vp_target * cos((float)i * 2.0 * PI * 1209.0 / (float)FS);
            float b = vp_target * cos((float)i * 2.0 * PI * 770.0 / (float)FS);
            float t = a + b;
            // Scale up to a int32 fixed 
            test1[i] = t;
        }

        // Long tone with a break in the middle
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(test1);
        detector.processBlock(test1 + N);
        detector.processBlock(test1 + 2 * N);
        detector.processBlock(test1 + 3 * N);
        detector.processBlock(silence);
        detector.processBlock(test1 + 4 * N);
        detector.processBlock(test1 + 5 * N);
        detector.processBlock(silence);
        detector.processBlock(silence);

        assert(!detector.isDetectionPending());
    }

    return 0;
}
