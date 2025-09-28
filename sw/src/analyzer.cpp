/**
 * Software Defined Repeater Controller
 * Copyright (C) 2025, Bruce MacKinnon KC1FSZ
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * NOT FOR COMMERCIAL USE WITHOUT PERMISSION.
 */

/*
When targeting RP2350 (Pico 2), command used to load code onto the board: 
~/git/openocd/src/openocd -s ~/git/openocd/tcl -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "rp2350.dap.core1 cortex_m reset_config sysresetreq" -c "program analyzer.elf verify reset exit"
*/

#include <cstdio>
#include <cstring>
#include <cmath>

#include <arm_math.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"

#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/WindowAverage.h"
#include "kc1fsz-tools/rp2040/PicoPollTimer.h"
#include "kc1fsz-tools/rp2040/PicoPerfTimer.h"
#include "kc1fsz-tools/rp2040/PicoClock.h"

#include "i2s_setup.h"

using namespace kc1fsz;

// ===========================================================================
// CONFIGURATION PARAMETERS
// ===========================================================================
//
#define LED0_PIN (PICO_DEFAULT_LED_PIN)
#define LED1_PIN (18)
#define R0_COS_PIN (14)
#define R0_CTCSS_PIN (13)
#define R0_PTT_PIN (12)
#define R1_COS_PIN (17)
#define R1_CTCSS_PIN (16)
#define R1_PTT_PIN (15)

// System clock rate
#define SYS_KHZ (153600)
#define WATCHDOG_INTERVAL_MS (2000)
#define FS_ADC 32000

// ===========================================================================
// DIAGNOSTIC COUNTERS/FLAGS
// ===========================================================================
//
static uint32_t longestLoop = 0;
extern uint32_t longestIsr;

// ===========================================================================
// RUNTIME OBJECTS
// ===========================================================================
//
static PicoClock clock;
static PicoPerfTimer perfTimerLoop;

static float32_t signalRms = 0;
static float32_t signalPeak = 0;

static float32_t toneHz = 2000;
static float32_t toneOmega = 0;
static float32_t tonePhi = 0;
static float32_t toneLevel = 0.25;

static float32_t dftBuffer[1024];
static arm_cfft_instance_f32 dftInstance;

/**
 * This is the callback that gets fired on every audio cycle. This will be 
 * called in an ISR context.
 */
static void audio_proc(const int32_t* r0_samples, const int32_t* r1_samples,
    int32_t* r0_out, int32_t* r1_out) {    

    float adc_in[ADC_SAMPLE_COUNT];
    float dac_out[ADC_SAMPLE_COUNT];

    // Convert fixed-point to floating point
    arm_q31_to_float(r0_samples, adc_in, ADC_SAMPLE_COUNT);
    // Compute the signal RMS/peak
    arm_rms_f32(adc_in, ADC_SAMPLE_COUNT, &signalRms);
    uint32_t signalPeakIndex;
    arm_absmax_f32(adc_in, ADC_SAMPLE_COUNT, &signalPeak, &signalPeakIndex);

    // Accumulate DFT buffer
    memmove(dftBuffer, dftBuffer + ADC_SAMPLE_COUNT, (1024 - ADC_SAMPLE_COUNT) * 4);
    memmove(dftBuffer + 3 * ADC_SAMPLE_COUNT, adc_in, ADC_SAMPLE_COUNT * 4);

    // Make tone
    for (unsigned i = 0; i < ADC_SAMPLE_COUNT; 
        i++, tonePhi += toneOmega) 
        dac_out[i] = toneLevel * arm_cos_f32(tonePhi);

    // We do this to avoid phi growing very large and 
    // creating overflow/precision problems.
    tonePhi = fmod(tonePhi, 2.0 * PI);

    // Convert back to fixed point
    arm_float_to_q31(dac_out, r0_out, ADC_SAMPLE_COUNT);
}

float mag_sq(float a, float b) {
    return a * a + b * b;
}

int main(int argc, const char** argv) {

    // Adjust system clock to more evenly divide the 
    // audio sampling frequency.
    set_sys_clock_khz(SYS_KHZ, true);

    stdio_init_all();

    gpio_init(LED0_PIN);
    gpio_set_dir(LED0_PIN, GPIO_OUT);
    gpio_put(LED0_PIN, 0);
    gpio_init(LED1_PIN);
    gpio_set_dir(LED1_PIN, GPIO_OUT);
    gpio_put(LED1_PIN, 0);
    
    gpio_init(R0_COS_PIN);
    gpio_set_dir(R0_COS_PIN, GPIO_IN);
    gpio_init(R0_CTCSS_PIN);
    gpio_set_dir(R0_CTCSS_PIN, GPIO_IN);
    gpio_init(R0_PTT_PIN);
    gpio_set_dir(R0_PTT_PIN, GPIO_OUT);
    // Logic is inverted, so 0 is off (not pulled to ground)
    gpio_put(R0_PTT_PIN, 0);
    gpio_init(R1_COS_PIN);
    gpio_set_dir(R1_COS_PIN, GPIO_IN);
    gpio_init(R1_CTCSS_PIN);
    gpio_set_dir(R1_CTCSS_PIN, GPIO_IN);
    gpio_init(R1_PTT_PIN);
    gpio_set_dir(R1_PTT_PIN, GPIO_OUT);
    // Logic is inverted, so 0 is off (not pulled to ground)
    gpio_put(R1_PTT_PIN, 0);

    // Startup indicator
    for (unsigned i = 0; i < 4; i++) {
        sleep_ms(200);
        gpio_put(LED0_PIN, 1);
        gpio_put(LED1_PIN, 1);
        sleep_ms(200);
        gpio_put(LED0_PIN, 0);
        gpio_put(LED1_PIN, 0);
    }

    Log log(&clock);
    log.setEnabled(true);

    log.info("Audio Analyzer");
    log.info("Copyright (C) 2025 Bruce MacKinnon KC1FSZ");

    if (watchdog_enable_caused_reboot()) {
        log.info("Rebooted by watchdog timer");
    } else {
        log.info("Clean boot");
    }
    
    // Enable the watchdog, requiring the watchdog to be updated or the chip 
    // will reboot. The second arg is "pause on debug" which means 
    // the watchdog will pause when stepping through code
    watchdog_enable(WATCHDOG_INTERVAL_MS, 1);

    // Enable audio processing 
    audio_setup(audio_proc);

    clock.reset();

    // Display/diagnostic should happen once per second
    PicoPollTimer flashTimer;
    flashTimer.setIntervalUs(50 * 1000);

    arm_cfft_init_f32(&dftInstance, 1024);

    float32_t hw[1024], hwS = 0;
    for (unsigned n = 0; n < 1024; n++) {
        hw[n] = 0.54 - 0.46 * cos(2.0 * PI * (float)n / 1023.0);
        hwS += hw[n];
    }

    enum State { PRE, SWEEP, POST } state = State::PRE;

    unsigned step = 1;
    const unsigned steps = 128;
    float sweepStepHz = 4000 / (float)steps;
    float sweepHz = sweepStepHz;
    float sweepStartHz = sweepStepHz;
    float sweepMags[steps] = { 0 };
    float sweepThds[steps] = { 0 };

    float sweepCal[steps] = { 0 };
    for (unsigned i = 0; i < steps; i++)
        sweepCal[i] = 1.0;

    // ===== Main Event Loop =================================================

    printf("\033[?25h");

    while (true) { 

        watchdog_update();

        perfTimerLoop.reset();

        int c = getchar_timeout_us(0);
        bool flash = flashTimer.poll();

        if (c == 'c') {
            printf("Calibration\n");
            for (unsigned i = 0; i < steps; i++)
                sweepCal[i] = sweepMags[i];
        }

        if (flash) {

            if (state == State::PRE) {
                step = 1;
                sweepHz = sweepStartHz;
                // Convert frequency to radians/sample.  
                toneOmega = 2.0 * PI * sweepHz / (float)FS_ADC;
                state = State::SWEEP;
                // Logic is inverted, so 1 is pulled to ground
                gpio_put(R0_PTT_PIN, 1);
            }
            else if (state == State::POST) {
                gpio_put(R0_PTT_PIN, 0);
                state = State::PRE;
            }
            else if (state == State::SWEEP) {

                // DFT
                float32_t dftIn[1024 * 2];
                // Build complex array w/ window
                for (unsigned i = 0; i < 1024; i++) {
                    dftIn[i * 2] = dftBuffer[i] * hw[i];
                    dftIn[i * 2 + 1] = 0;
                }
                arm_cfft_f32(&dftInstance, dftIn, 0, 1);

                float32_t dftMax = 0;
                unsigned dftMaxBin = 0;

                // Find the fundmamental frequency
                for (unsigned i = 1; i < 1024 / 2; i++) {
                    float32_t mag2 = mag_sq(dftIn[i * 2], dftIn[i * 2 + 1]);
                    if (mag2 > dftMax) {
                        dftMaxBin = i;
                        dftMax = mag2;
                    }
                }

                // Total up the harmonics
                float32_t harmonicSum = 0;
                for (unsigned h = 2; h < 32; h++) {
                    unsigned hBin = dftMaxBin * h;
                    // Are we off the end?
                    if (hBin >= 1024 / 2)
                        break;
                    float32_t r = dftIn[hBin * 2];
                    r = r * r;
                    float32_t c = dftIn[hBin * 2 + 1];
                    c = c * c;
                    float32_t mag = sqrt(r + c);
                    float32_t rms = mag * 0.7071;
                    harmonicSum += rms * rms;
                }
                float32_t thd = sqrt(harmonicSum) / (dftMax * 0.7071);

                float dbPeak = signalPeak > 0.001 ? 20.0 * log10(signalPeak * 2.0) : -99;            
                float maxM = (2.0) * sqrt(dftMax) / hwS;
                float maxF = 32000.0 * (float)dftMaxBin / 1024.0;

                sweepMags[step] = maxM;
                sweepThds[step] = thd;
                //sweepMags[step] = signalRms;

                // Check for end of sweep
                if (step == steps) {
                    printf("SWEEP %f %f ", sweepStartHz, sweepStepHz);
                    for (unsigned n = 1; n < steps; n++)
                        printf("%.3f ", sweepMags[n] / sweepCal[n]);
                    printf("\n");
                    //printf("THD %f %f ", sweepStartHz, sweepStepHz);
                    //for (unsigned n = 1; n < steps; n++)
                    //    printf("%.2f ", sweepThds[n] * 100.0);
                    printf("\n");
                    state = State::POST;
                }
                else {
                    // Advance
                    step++;
                    sweepHz += sweepStepHz;
                    // Convert frequency to radians/sample.  
                    toneOmega = 2.0 * PI * sweepHz / (float)FS_ADC;
                }
            }
        }

        uint32_t t = perfTimerLoop.elapsedUs();
        if (t > longestLoop)
            longestLoop = t;
    }
}
