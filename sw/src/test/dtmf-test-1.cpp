/*
A unit test of the DTMF detector
*/
#include <iostream>
#include <cmath>
#include <cassert>
#include <random>

#include <arm_math.h>
#include "DTMFDetector2.h"

using namespace std;
using namespace kc1fsz;

/*
// Function to generate a block of white noise samples
// NOTE: If the mean of the noise is zero, RMS noise and standard deviation 
// are equivalent! 
//
static void generateWhiteNoise(unsigned int numSamples, double amplitude, float* out) {
    float std = sqrt(pow(2, 2 * amplitude) / 12);
    cout << "STD " << std << endl;
    // Obtain a random number from hardware
    std::random_device rd;
    // Seed the generator
    std::mt19937 gen(rd()); 
    std::uniform_real_distribution<> distrib(-amplitude, amplitude);
    for (unsigned int i = 0; i < numSamples; ++i)
        out[i] = distrib(gen);
}
        */

static void addWhiteNoise(float* out, unsigned n, float std) {
    // Obtain a random number from hardware
    std::random_device rd;
    std::default_random_engine gen(rd()); 
    // Seed the generator
    //std::mt19937 gen(rd()); 
    //std::uniform_real_distribution<> distrib(-amp, amp);
    // Create a normal distribution object with a mean of 0 and the desired standard deviation
    std::normal_distribution<double> distrib(0.0, std);

    for (unsigned int i = 0; i < n; i++)
        out[i] += distrib(gen);
}

int main(int,const char**) {

    const unsigned FS = 8000;
    const unsigned N = 64;
    const float noise = 0.01;
    const float threshold = -55;
    const float thresholdLinear = pow(10.0, (threshold/20.0));

    printf("Noise STD %f\n", noise);
    printf("Vrms detection threshold %f\n", thresholdLinear);

    DTMFDetector2 detector;
    detector.setSignalThreshold(threshold);

    float silence[N];
    for (unsigned int i = 0; i < N; i++) 
        silence[i] = 0;
    addWhiteNoise(silence, N, noise);
    // Compute the RMS of the silence
    float rt = 0;
    for (unsigned int i = 0; i < N; i++) 
        rt += silence[i] * silence[i];
    rt = sqrt(rt);
    printf("Silence Vrms %f\n", rt);
    printf("Silence Vrms**s %f\n", rt * rt);

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
        addWhiteNoise(test1, N * 6, noise);

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

        assert(detector.isDetectionPending());
        assert(detector.popDetection() == '4');
        assert(!detector.isDetectionPending());
    }

    {
        cout << "----- Test 1a (Unacceptable twist col > row) ------" << endl;
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
        addWhiteNoise(test1, N * 6, noise);

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

        assert(!detector.isDetectionPending());
    }

    {
        cout << "----- Test 1b (Acceptable twist, col > row) ------" << endl;
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
        addWhiteNoise(test1, N * 6, noise);

        // 40ms tone w/ 40ms silence (valid DSC)
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
        addWhiteNoise(test1, N * 6, noise);

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
        addWhiteNoise(test1, N * 6, noise);

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

        assert(detector.isDetectionPending());
        assert(detector.popDetection() == '*');
        assert(!detector.isDetectionPending());
    }

    {
        cout << "----- Test 2 (Bad freq) ------" << endl;
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
        addWhiteNoise(testBad, N * 6, noise);

        // 40ms bad tone w/ 40ms silence (not valid DSC)
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);

        detector.processBlock(testBad);
        detector.processBlock(testBad + N);
        detector.processBlock(testBad + N * 2);
        detector.processBlock(testBad + N * 3);
        detector.processBlock(testBad + N * 4);
        detector.processBlock(testBad + N * 5);

        assert(!detector.isDetectionPending());
    }

    {
        cout << "----- Test 3 (Too short) ------" << endl;
        float test1[N * 6];
        for (unsigned int i = 0; i < N * 6; i++) {
            // Two valid tones
            float a = vp_target * cos((float)i * 2.0 * PI * 1209.0 / (float)FS);
            float b = vp_target * cos((float)i * 2.0 * PI * 770.0 / (float)FS);
            float t = a + b;
            // Scale up to a int32 fixed 
            test1[i] = t;
        }
        addWhiteNoise(test1, N * 6, noise);

        // 20ms tone w/ 40ms silence (not valid DSC)
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(test1);
        detector.processBlock(test1 + N);
        detector.processBlock(test1 + 2 * N);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);

        assert(!detector.isDetectionPending());
    }

    {
        cout << "----- Test 4 (Tone with break) ------" << endl;
        float test1[N * 6];
        for (unsigned int i = 0; i < N * 6; i++) {
            // Two valid tones
            float a = vp_target * cos((float)i * 2.0 * PI * 1209.0 / (float)FS);
            float b = vp_target * cos((float)i * 2.0 * PI * 770.0 / (float)FS);
            float t = a + b;
            test1[i] = t;
        }
        addWhiteNoise(test1, N * 6, noise);

        // Long tone with a break in the middle
        detector.processBlock(silence);
        detector.processBlock(silence);
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

    {
        cout << "----- Test 5 (Valid tone with break) ------" << endl;
        float test1[N * 6];
        for (unsigned int i = 0; i < N * 6; i++) {
            // Two valid tones
            float a = vp_target * cos((float)i * 2.0 * PI * 1209.0 / (float)FS);
            float b = vp_target * cos((float)i * 2.0 * PI * 770.0 / (float)FS);
            float t = a + b;
            test1[i] = t;
        }
        addWhiteNoise(test1, N * 6, noise);

        // Long tone with a break in the middle
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);
        detector.processBlock(silence);

        detector.processBlock(test1);
        detector.processBlock(test1 + N);
        detector.processBlock(test1 + 2 * N);
        detector.processBlock(test1 + 3 * N);
        detector.processBlock(test1 + 4 * N);
        detector.processBlock(test1 + 5 * N);
        // Pick up the detection
        assert(detector.isDetectionPending());
        assert(detector.popDetection() == '4');

        // Short gap
        detector.processBlock(silence);

        // Keep going
        detector.processBlock(test1);
        detector.processBlock(test1 + N);
        detector.processBlock(test1 + 2 * N);
        detector.processBlock(test1 + 3 * N);
        detector.processBlock(test1 + 4 * N);
        detector.processBlock(test1 + 5 * N);
        detector.processBlock(test1);
        detector.processBlock(test1 + N);

        // No new detection
        assert(!detector.isDetectionPending());
    }

    return 0;
}
