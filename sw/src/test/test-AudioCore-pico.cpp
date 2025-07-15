/*
When targeting RP2350 (Pico 2), command used to load code onto the board: 
~/git/openocd/src/openocd -s ~/git/openocd/tcl -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "rp2350.dap.core1 cortex_m reset_config sysresetreq" -c "program main.elf verify reset exit"
*/
#include <cstdio>
#include <cstring>
#include <cmath>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/watchdog.h"

#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/rp2040/PicoClock.h"
#include "kc1fsz-tools/rp2040/PicoPerfTimer.h"

// Radlib
#include "util/dsp_util.h"

#include "AudioCore.h"

#define LED_PIN (PICO_DEFAULT_LED_PIN)

using namespace kc1fsz;
using namespace radlib;

static PicoClock clock;

static float dB(float l) {
    return 20.0 * log10(l);
}

int main(int, const char**) {

    // Adjust system clock to more evenly divide the 
    // audio sampling frequency.
    unsigned long system_clock_khz = 129600;
    set_sys_clock_khz(system_clock_khz, true);

    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);   

    // Startup ID
    sleep_ms(500);
    gpio_put(LED_PIN, 1);
    sleep_ms(500);
    gpio_put(LED_PIN, 0);

    Log log(&clock);
    log.setEnabled(true);

    sleep_ms(50);

    log.info("W1TKZ Software Defined Repeater Controller");
    log.info("Copyright (C) 2025 Bruce MacKinnon KC1FSZ");
    log.info("AudioCore size in bytes %d", sizeof(AudioCore));

    AudioCore core0(0);
    AudioCore core1(1);
    core0.setCtcssDecodeFreq(123);
    core0.setCtcssEncodeFreq(123);
    core0.setCtcssEncodeLevel(-26);
    core0.setCtcssEncodeEnabled(true);
    core0.setDelayMs(100);

    // Create 230ms of test data for each radio
    // (32,000 / 4) * 4 * 2 = 64,000
    const unsigned test_in_max = AudioCore::FS_ADC / 4;
    const unsigned test_blocks = test_in_max / AudioCore::BLOCK_SIZE_ADC;
    float test_in_0[test_in_max];
    float test_in_1[test_in_max];
    for (unsigned i = 0; i < test_in_max; i++) {
        test_in_0[i] = 0;
        test_in_1[i] = 0;
    }

    // Fill in the test audio
    //generateWhiteNoise(test_in_len / 2, 1.0, test_in_0);
    // PL
    //int ft = 123;
    // Noise
    //int ft = 6000;
    // Audio
    int ft = 123;
    make_real_tone_f32(test_in_0, test_in_max, AudioCore::FS_ADC, ft, 0.5); 
    //unsigned test_in_0_len = loadFromFile("../tests/clip-3.txt", test_in_0, test_in_max);

    float* adc_in_0 = test_in_0;
    float* adc_in_1 = test_in_1;

    float cross_out_0[AudioCore::BLOCK_SIZE];
    float cross_out_1[AudioCore::BLOCK_SIZE];
    const float* cross_ins[2] = { cross_out_0, cross_out_1 };
    float cross_gains[2] = { 1.0, 0.0 };

    float dac_out_0[AudioCore::BLOCK_SIZE_ADC];
    float dac_out_1[AudioCore::BLOCK_SIZE_ADC];

    // Run an Audio cycle with timing
    PicoPerfTimer timer;

    for (unsigned block = 0; block < 8; block++) {

        log.info("Block %d", block);

        timer.reset();

        core0.cycle0(adc_in_0, cross_out_0);
        core1.cycle0(adc_in_1, cross_out_1);
        core0.cycle1(2, cross_ins, cross_gains, dac_out_0);
        core1.cycle1(2, cross_ins, cross_gains, dac_out_1);

        adc_in_0 += AudioCore::BLOCK_SIZE_ADC;
        adc_in_1 += AudioCore::BLOCK_SIZE_ADC;

        float np = dB(core0.getNoiseRms());
        float sp = dB(core0.getSignalRms());
        float op = dB(core0.getOutRms());
        float cm = dB(core0.getCtcssDecodeMag());
        unsigned elUs = timer.elapsedUs();

        log.info("  Elapsed us      %u", elUs);
        log.info("  Noise power dB  %f", np);
        log.info("  Signal power dB %f", sp);
        log.info("  Output power dB %f", op);
        log.info("  CTCSS power dB  %f", cm);
    }

    while (true) {
    }
}

