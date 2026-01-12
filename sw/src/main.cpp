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
~/git/openocd/src/openocd -s ~/git/openocd/tcl -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "rp2350.dap.core1 cortex_m reset_config sysresetreq" -c "program main.elf verify reset exit"
*/

#include <cstdio>
#include <cstring>
#include <cmath>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"

#include "hardware/dma.h"
#include "hardware/uart.h"

#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/StdPollTimer.h"
#include "kc1fsz-tools/CommandShell.h"
#include "kc1fsz-tools/OutStream.h"
#include "kc1fsz-tools/WindowAverage.h"
#include "kc1fsz-tools/DTMFDetector2.h"
#include "kc1fsz-tools/rp2040/PicoPerfTimer.h"
#include "kc1fsz-tools/rp2040/PicoClock.h"

#include "StdTx.h"
#include "StdRx.h"
#include "Config.h"
#include "TxControl.h"
#include "ShellCommand.h"
#include "AudioCore.h"
#include "AudioCoreOutputPortStd.h"
#include "CommandProcessor.h"
#include "DigitalPort.h"

#include "i2s_setup.h"

using namespace kc1fsz;

// ===========================================================================
// CONFIGURATION PARAMETERS
// ===========================================================================
//
static const char* VERSION = "V1.2 2026-01-12";
#define LED_PIN (PICO_DEFAULT_LED_PIN)
#define R0_COS_PIN (14)
#define R0_CTCSS_PIN (13)
#define R0_PTT_PIN (12)
#define R1_COS_PIN (17)
#define R1_CTCSS_PIN (16)
#define R1_PTT_PIN (15)
#define LED2_PIN (18)

// System clock rate
#define SYS_KHZ (153600)
#define WATCHDOG_INTERVAL_MS (2000)

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
// The global configuration parameters
static Config config;

static PicoClock clock;
static PicoPerfTimer perfTimerLoop;

static AudioCore core0(0, 3, clock);
static AudioCore core1(1, 3, clock);
// This core is the digital audio input port
static DigitalPort core2(2, 3, clock);

// The console can work in one of three modes:
// 
// Log    - A stream of log/diagnostic messages (default)
// Shell  - An interactive command prompt
// Status - A continuously updated live status page
//
enum UIMode { UIMODE_LOG, UIMODE_SHELL, UIMODE_STATUS };

// Used for integrating with the command shell
class ShellOutput : public OutStream {
public:

    /**
     * @returns Number of bytes actually written.
     */
    int write(uint8_t b) { printf("%c", (char)b); return 1; }
    bool isWritable() const { return true; }
};

/**
 * This is the callback that gets fired on every audio cycle. This will be 
 * called in an ISR context.
 */
static void audio_proc(const int32_t* r0_samples, const int32_t* r1_samples,
    int32_t* r0_out, int32_t* r1_out) {
    
    float r0_cross[ADC_SAMPLE_COUNT / 4];
    float r1_cross[ADC_SAMPLE_COUNT / 4];
    float r2_cross[ADC_SAMPLE_COUNT / 4];
    const float* cross_ins[3] = { r0_cross, r1_cross, r2_cross };

    core0.cycleRx(r0_samples, r0_cross);
    core1.cycleRx(r1_samples, r1_cross);
    core2.cycleRx(r2_cross);
    core0.cycleTx(cross_ins, r0_out);
    core1.cycleTx(cross_ins, r1_out);
    core2.cycleTx(cross_ins);
}

static void print_bar(float vrms, float vpeak) {
    // Scale to VU standard
    float min = 0.001;
    float dbrms = vrms > min ? 20.0 * log10(vrms * 2.0) : -99;
    float dbpeak = vpeak > min ? 20.0 * log10(vpeak * 2.0) : -99;
    float vrms_vu = vrms * 11.06;
    float dbrms_vu = vrms_vu > min ? 20.0 * log10(vrms_vu / 1.22799537) : -99;
    float vpeak_vu = vpeak * 11.06;
    float dbpeak_vu = vpeak_vu > min ? 20.0 * log10(vpeak_vu / 1.22799537) : -99;

    float rms_ticks = (vrms_vu / 1.227) * 10.0;
    if (rms_ticks > 14)
        rms_ticks = 14;
    float peak_ticks = (vpeak_vu / 1.227) * 10.0;
    if (peak_ticks > 14)
        peak_ticks = 14;
    printf("[\033[32m");
    for (int i = 0; i < 15; i++) {
        if (i == 10)
            printf("\033[31m");
        if (i == (int)peak_ticks) 
            printf("|");
        else if (i < rms_ticks)
            printf("=");
        else 
            printf(" ");
    }
    printf("\033[37m] %5.1f VU (Pk %5.1f) %5.1f dBFS (Pk %5.1f)   ", 
        dbrms_vu, dbpeak_vu, dbrms, dbpeak);
    printf("\033[0m");
}

static void render_status(const Rx& rx0, const Rx& rx1, const Tx& tx0, const Tx& tx1,
    const TxControl& txc0, const TxControl& txc1) {

    printf("\033[H");
    printf("W1TKZ Software Defined Repeater Controller (%s)\n", VERSION);
    printf("\n");

    printf("\033[30;47m");
    printf(" Radio 0 ");
    printf("\033[0m");
    if (tx0.getEnabled()) 
        printf(" TX ENABLED   \n");
    else 
        printf(" TX DISABLED  \n");

    printf("RX0 COS  : ");
    if (rx0.isCOS()) {
        printf("\033[30;42m");
        printf("ACTIVE  ");
    } else {
        printf("\033[2m");
        printf("INACTIVE");
    }
    printf("\n");
    printf("\033[0m");

    printf("RX0 CTCSS: ");
    if (rx0.isCTCSS()) {
        printf("\033[30;42m");
        printf("ACTIVE  ");
    } else {
        printf("\033[2m");
        printf("INACTIVE");
    }
    printf("\n");
    printf("\033[0m");

    printf("TX0 PTT  : ");
    if (tx0.getPtt()) {
        printf("\033[30;42m");
        printf("ACTIVE  ");
    } else {
        printf("\033[2m");
        printf("INACTIVE");
    }
    printf("\n");
    printf("\033[0m");

    printf("RX0 LVL  : ");
    print_bar(core0.getSignalRms2(), core0.getSignalPeak2());
    printf("\n");

    printf("TX0 LVL  : ");
    print_bar(core0.getOutRms2(), core0.getOutPeak2());
    printf("\n");
    printf("Tone RMS: %.2f, Noise RMS: %.2f, Signal RMS: %.2f, SNR: %.1f  \n", 
        core0.getCtcssDecodeRms(), 
        core0.getNoiseRms(), core0.getSignalRms2(),
        AudioCore::db(core0.getSignalRms() / core0.getNoiseRms()));
    printf("Tone dBFS: %f\n", AudioCore::vrmsToDbv(core0.getCtcssDecodeRms()));
    printf("AGC gain: %.1f\n", AudioCore::db(core0.getAgcGain()));
    printf("\n");
                
    printf("\033[30;47m");
    printf(" Radio 1 ");
    printf("\033[0m");
    if (tx1.getEnabled()) 
        printf(" TX ENABLED   \n");
    else 
        printf(" TX DISABLED  \n");

    printf("RX1 COS  : ");
    if (rx1.isCOS()) {
        printf("\033[30;42m");
        printf("ACTIVE  ");
    } else {
        printf("\033[2m");
        printf("INACTIVE");
    }
    printf("\n");
    printf("\033[0m");

    printf("RX1 CTCSS: ");
    if (rx1.isCTCSS()) {
        printf("\033[30;42m");
        printf("ACTIVE  ");
    } else {
        printf("\033[2m");
        printf("INACTIVE");
    }
    printf("\n");
    printf("\033[0m");

    printf("TX1 PTT  : ");
    if (tx1.getPtt()) {
        printf("\033[30;42m");
        printf("ACTIVE  ");
    } else {
        printf("\033[2m");
        printf("INACTIVE");
    }
    printf("\n");
    printf("\033[0m");

    printf("RX1 LVL  : ");
    print_bar(core1.getSignalRms2(), core1.getSignalPeak2());
    printf("\n");

    printf("TX1 LVL  : ");
    print_bar(core1.getOutRms2(), core1.getOutPeak2());
    printf("\n");
    printf("Tone RMS: %.2f, Noise RMS: %.2f, Signal RMS: %.2f, SNR: %.1f  \n", 
        core1.getCtcssDecodeRms(), 
        core1.getNoiseRms(), 
        core1.getSignalRms(),
        AudioCore::db(core1.getSignalRms() / core1.getNoiseRms()));
    printf("Tone dBFS: %f\n", AudioCore::vrmsToDbv(core1.getCtcssDecodeRms()));
    printf("AGC gain: %.1f\n", AudioCore::db(core1.getAgcGain()));
    printf("\n");

    printf("%u / %u / %d / %d      \n", longestIsr, longestLoop, txc0.getState(), txc1.getState());
}

static void transferConfigRx(const Config::ReceiveConfig& config, Rx& rx) {
    rx.setCosMode((Rx::CosMode)config.cosMode);
    rx.setCosActiveTime(config.cosActiveTime);
    rx.setCosInactiveTime(config.cosInactiveTime);
    rx.setCosLevel(config.cosLevel);
    rx.setToneMode((Rx::ToneMode)config.toneMode);
    rx.setToneActiveTime(config.toneActiveTime);
    rx.setToneInactiveTime(config.toneInactiveTime);
    rx.setToneLevel(config.toneLevel);
    rx.setToneFreq(config.toneFreq);
    rx.setGainLinear(AudioCore::dbToLinear(config.gain));
    rx.setDelayTime(config.delayTime);
    rx.setDtmfDetectLevel(config.dtmfDetectLevel);
    rx.setDeemphMode(config.deemphMode);
}

static void transferConfigTx(const Config::TransmitConfig& config, Tx& tx) {
    tx.setEnabled((bool)config.enabled);
    tx.setPLToneMode((Tx::PLToneMode)config.toneMode);
    tx.setPLToneLevel(config.toneLevel);
    tx.setPLToneFreq(config.toneFreq);
    tx.setCtMode((CourtesyToneGenerator::Type)config.ctMode);
}

static void transferControlConfig(const Config::ControlConfig& config, TxControl& txc) {
    txc.setTimeoutTime(config.timeoutTime);
    txc.setLockoutTime(config.lockoutTime);
    txc.setHangTime(config.hangTime);
    txc.setCtLevel(config.ctLevel);
    txc.setIdMode(config.idMode);
    txc.setIdLevel(config.idLevel);
    // At the moment, all receivers are eligible
    //for (unsigned i = 0; i < Config::maxReceivers; i++)
    //    txc.setRxEligible(i, config.rxEligible[i]);
}

/**
 * @brief Transfers configuration parameters from the 
 * Config structure into the actual repeater controller.
 * This needs to happen once at started and then any 
 * time that the configuration is changed.
 */
static void transferConfig(const Config& config,
    Rx& rx0, Rx& rx1,
    Tx& tx0, Tx& tx1,
    TxControl& txc0, TxControl& txc1,
    AudioCore& core0, AudioCore& core1) 
{
    // General configuration
    txc0.setCall(config.general.callSign);
    txc0.setPass(config.general.pass);
    txc0.setIdRequiredInt(config.general.idRequiredInt);
    txc0.setDiagToneFreq(config.general.diagFreq);
    txc0.setDiagToneLevel(config.general.diagLevel);

    txc1.setCall(config.general.callSign);
    txc1.setPass(config.general.pass);
    txc1.setIdRequiredInt(config.general.idRequiredInt);
    txc1.setDiagToneFreq(config.general.diagFreq);
    txc1.setDiagToneLevel(config.general.diagLevel);

    // Receiver configuration
    transferConfigRx(config.rx0, rx0);
    transferConfigRx(config.rx1, rx1);

    // Transmitter configuration
    transferConfigTx(config.tx0, tx0);
    transferConfigTx(config.tx1, tx1);

    // Controller configuration
    transferControlConfig(config.txc0, txc0);
    transferControlConfig(config.txc1, txc1);
}

#define BUF_SIZE 256
static uint8_t rx_buf[BUF_SIZE];
static uint8_t tx_buf[BUF_SIZE];

static volatile unsigned rx_state = 0;
static unsigned packet_size = 133;

// ------ UART Setup Stuff -------------------------------------------------

static uint dma_ch_tx;
static uint dma_ch_rx;

static void dma_tx_handler() {
}

// IMPORTANT NOTE: This is running in an interrupt so move quickly!!!
static void process_rx_message(const uint8_t* rxData, unsigned rxDataLen) {   
    // TEMPORARY ECHO
    memcpy(tx_buf, rxData, rxDataLen);
    // Fire the outbound transfer for the full body
    dma_channel_transfer_from_buffer_now(dma_ch_tx, tx_buf, rxDataLen);
}

// IMPORTANT NOTE: This is running in an interrupt so move quickly!!!
static void dma_rx_handler() {    
    // Waiting for header?
    if (rx_state == 0) {
        if (rx_buf[0] == 0) {
            rx_state = 1;
            // Restart the inbound transfer for the full body
            dma_channel_transfer_to_buffer_now(dma_ch_rx, rx_buf, packet_size);
        } else {
            // Restart the inbound transfer for the header byte
            dma_channel_transfer_to_buffer_now(dma_ch_rx, rx_buf, 1);
        }
    }
    // Waiting for body?
    else if (rx_state == 1) {
        // Decode the packet
        // Make sure there are no header bytes in the buffer
        bool fault = false;
        //for (unsigned i = 0; i < packet_size && !fault; i++)
        //    if (rx_buf[i] == 0)
        //        fault = true;
        if (!fault) 
            process_rx_message(rx_buf, packet_size);
        // Restart the inbound transfer for the next header byte
        dma_channel_transfer_to_buffer_now(dma_ch_rx, rx_buf, 1);
        rx_state = 0;
    }
}

// IMPORTANT NOTE: This is running in an interrupt so move quickly!!!
static void dma_irq1_handler() {   
    if (dma_channel_get_irq1_status(dma_ch_tx)) {
        dma_channel_acknowledge_irq1(dma_ch_tx);
        dma_tx_handler();
    }
    if (dma_channel_get_irq1_status(dma_ch_rx)) {
        dma_channel_acknowledge_irq1(dma_ch_rx);
        dma_rx_handler();
    }
}

static void streaming_uart_setup() {

    uart_init(uart0, 1152000);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);
  
    dma_ch_rx = dma_claim_unused_channel(true);
    dma_channel_config c_rx = dma_channel_get_default_config(dma_ch_rx);
    channel_config_set_transfer_data_size(&c_rx, DMA_SIZE_8);
    // Always reading from static UART register
    channel_config_set_read_increment(&c_rx, false); 
    // Writing to increasing memory address    
    channel_config_set_write_increment(&c_rx, true); 
    channel_config_set_dreq(&c_rx, DREQ_UART0_RX);
    channel_config_set_enable(&c_rx, true);

    // For a circular buffer, use channel_config_set_wrap() and 
    // dma_channel_set_write_addr() to restart the transfer when the 
    // buffer limit is reached.
    dma_channel_configure(
        dma_ch_rx,
        &c_rx,
        // Destination address        
        rx_buf,
        &uart_get_hw(uart0)->dr, // Source address (UART Data Register)
        // First transfer is for the header only
        1,
        false);
    dma_channel_set_irq1_enabled(dma_ch_rx, true);

    dma_ch_tx = dma_claim_unused_channel(true);
    dma_channel_config c_tx = dma_channel_get_default_config(dma_ch_tx);
    channel_config_set_transfer_data_size(&c_tx, DMA_SIZE_8);
    // Always write to static UART register
    channel_config_set_write_increment(&c_tx, false); 
    // Reading from increasing memory address    
    channel_config_set_read_increment(&c_tx, true); 
    channel_config_set_dreq(&c_tx, DREQ_UART0_TX);
    channel_config_set_enable(&c_tx, true);
    
    dma_channel_configure(
        dma_ch_tx,
        &c_tx,
        &uart_get_hw(uart0)->dr,
        tx_buf,
        BUF_SIZE,
        false);
    dma_channel_set_irq1_enabled(dma_ch_tx, true);

    // Bind to the interrupt handler 
    irq_set_exclusive_handler(DMA_IRQ_1, dma_irq1_handler);
    irq_set_enabled(DMA_IRQ_1, true);
    
    // Start the ball rolling on the RX side
    rx_state = 0;
    dma_channel_start(dma_ch_rx);
}

int main(int argc, const char** argv) {

    // Adjust system clock to more evenly divide the 
    // audio sampling frequency.
    set_sys_clock_khz(SYS_KHZ, true);

    // Very high speed
    stdio_uart_init_full(uart0, 1152000, 0, 1);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(LED2_PIN);
    gpio_set_dir(LED2_PIN, GPIO_OUT);
    
    gpio_init(R0_COS_PIN);
    gpio_set_dir(R0_COS_PIN, GPIO_IN);
    gpio_init(R0_CTCSS_PIN);
    gpio_set_dir(R0_CTCSS_PIN, GPIO_IN);
    gpio_init(R0_PTT_PIN);
    gpio_set_dir(R0_PTT_PIN, GPIO_OUT);
    gpio_put(R0_PTT_PIN, 0);
    gpio_init(R1_COS_PIN);
    gpio_set_dir(R1_COS_PIN, GPIO_IN);
    gpio_init(R1_CTCSS_PIN);
    gpio_set_dir(R1_CTCSS_PIN, GPIO_IN);
    gpio_init(R1_PTT_PIN);
    gpio_set_dir(R1_PTT_PIN, GPIO_OUT);
    gpio_put(R1_PTT_PIN, 0);

    // Startup ID
    sleep_ms(500);
    gpio_put(LED_PIN, 1);
    sleep_ms(500);
    gpio_put(LED_PIN, 0);

    UIMode uiMode = UIMode::UIMODE_LOG;
    Log log(&clock);
    log.setEnabled(true);

    log.info("W1TKZ Software Defined Repeater Controller");
    log.info("Copyright (C) 2025 Bruce MacKinnon KC1FSZ");
    log.info("Firmware %s", VERSION);

    if (watchdog_enable_caused_reboot()) {
        log.info("Rebooted by watchdog timer");
    } else {
        log.info("Clean boot");
    }

    // ----- READ CONFIGURATION FROM FLASH ------------------------------------
    Config::loadConfig(&config);
    if (!config.isValid()) {
        log.info("Invalid config, setting factory default");
        Config::setFactoryDefaults(&config);
        Config::saveConfig(&config);
    }

    // Enable the watchdog, requiring the watchdog to be updated or the chip 
    // will reboot. The second arg is "pause on debug" which means 
    // the watchdog will pause when stepping through code. From SDK docs:
    //
    // pause_on_debug: If set to true, the watchdog will pause its countdown 
    // when a debugger is connected and active. This is useful during development 
    // to prevent unexpected resets.
    //watchdog_enable(WATCHDOG_INTERVAL_MS, 1);
    // In production we turn this feature off
    watchdog_enable(WATCHDOG_INTERVAL_MS, 0);

    // Enable audio processing 
    audio_setup(audio_proc);

    int strobe = 0;
    bool liveDisplay = false;
    bool liveLED = false;

    clock.reset();

    // Display/diagnostic should happen twice per second
    StdPollTimer flashTimer(clock, 500 * 1000);

    StdTx tx0(clock, log, 0, R0_PTT_PIN, core0,
        // IMPORTANT SAFETY MECHANISM: Polled to control keying
        []() {
            return config.tx0.enabled2;
        }
    );
    StdTx tx1(clock, log, 1, R1_PTT_PIN, core1,
        // IMPORTANT SAFETY MECHANISM: Polled to control keying
        []() {
            return config.tx1.enabled2;
        }
    );

    StdRx rx0(clock, log, 0, R0_COS_PIN, R0_CTCSS_PIN, core0);
    StdRx rx1(clock, log, 1, R1_COS_PIN, R1_CTCSS_PIN, core1);

    // #### TODO: REVIEW THIS TEMPORARY BRIDGE CLASS
    AudioCoreOutputPortStd acop0(core0, rx0, rx1, core2);
    AudioCoreOutputPortStd acop1(core1, rx0, rx1, core2);

    TxControl txCtl0(clock, log, tx0, acop0);
    TxControl txCtl1(clock, log, tx1, acop1);

    int i = 0;

    ShellOutput shellOutput;
    ShellCommand shellCommand(config, 
        // Log trigger
        [&uiMode, &log]() {
            uiMode = UIMode::UIMODE_LOG;
            log.setEnabled(true);
            log.info("Entered log mode");
        },
        // Status trigger
        [&uiMode, &log]() {
            // Clear off the status screen
            printf("\033[2J");
            // Hide cursor
            printf("\033[?25l");
            uiMode = UIMode::UIMODE_STATUS;
            log.setEnabled(false);
        },
        // Config change trigger
        [&rx0, &rx1, &tx0, &tx1, &txCtl0, &txCtl1, &log]() {
            // If anything in the configuration structure is 
            // changed then we force a transfer of all config
            // parameters from the config structure and into 
            // the controller objects.
            log.info("Transferring configuration");
            transferConfig(config, rx0, rx1, tx0, tx1, txCtl0, txCtl1, core0, core1);
        },
        // ID trigger
        [&txCtl0, &txCtl1, &log]() {
            txCtl0.forceId();
            txCtl1.forceId();        
        },
        // Test start trigger
        [&txCtl0, &txCtl1](int r) {
            if (r == 0)
                txCtl0.startTest();
            else if (r == 1)
                txCtl1.startTest();
        },
        // Test stop trigger
        [&txCtl0, &txCtl1](int r) {
            if (r == 0)
                txCtl0.stopTest();
            else if (r == 1)
                txCtl1.stopTest();
        }
        );

    CommandShell shell;
    shell.setOutput(&shellOutput);
    shell.setSink(&shellCommand);

    // DTMF Command processing
    CommandProcessor dtmfCmdProc(log, clock);
    dtmfCmdProc.setAccessTrigger([&log, &txCtl0, &txCtl1](bool enabled) {
        if (enabled) {
            log.info("Access enabled");
        } else {
            log.info("Access disabled");
        }
    });
    dtmfCmdProc.setDisableTrigger([&log]() {
        log.info("Disable");
        config.tx0.enabled2 = false;
        config.tx1.enabled2 = false;
        // Make sure these settings are non-volatile
        Config::saveConfig(&config);
    });
    dtmfCmdProc.setReenableTrigger([&log]() {
        log.info("Reenable");
        config.tx0.enabled2 = true;
        config.tx1.enabled2 = true;
        // Make sure these settings are non-volatile
        Config::saveConfig(&config);
    });
    dtmfCmdProc.setForceIdTrigger([&txCtl0, &txCtl1]() {
        txCtl0.forceId();
        txCtl1.forceId();
    });

    // Force initial config transfer
    transferConfig(config, rx0, rx1, tx0, tx1, txCtl0, txCtl1, core0, core1);

    // ===== Main Event Loop =================================================

    printf("\033[?25h");

    while (true) { 

        watchdog_update();

        perfTimerLoop.reset();

        int c = getchar_timeout_us(0);
        bool flash = flashTimer.poll();

        if (uiMode == UIMode::UIMODE_LOG) {
            if (c == 27) {
                uiMode = UIMode::UIMODE_SHELL;
                log.setEnabled(false);
                shell.reset();
            } else if (c == 's') {
                // Clear off the status screen
                printf("\033[2J");
                // Hide cursor
                printf("\033[?25l");
                uiMode = UIMode::UIMODE_STATUS;
                log.setEnabled(false);
            } else if (c == 'i') {
                txCtl0.forceId();
                txCtl1.forceId();
            } else if (c == 'a') {
                // Enter streaming mode
                stdio_uart_deinit();
                streaming_uart_setup();
            }
            //if (flash)
            //    printf("DTMF diag %f\n", core1.getDtmfDetectDiagValue());
        }
        else if (uiMode == UIMode::UIMODE_SHELL) {
            if (c != 0) {
                shell.process(c);
            }
        }
        else if (uiMode == UIMode::UIMODE_STATUS) {
            // Do periodic display/diagnostic stuff
            if (flash)
                render_status(rx0, rx1, tx0, tx1, txCtl0, txCtl1);
            if (c == 'l') {
                // Clear off the status screen
                printf("\033[2J");
                // Show cursor
                printf("\033[?25h");
                uiMode = UIMode::UIMODE_LOG;
                log.setEnabled(true);
                log.info("Entered log mode");
            } 
            else if (c == 27) {
                // Clear off the status screen
                printf("\033[2J");
                // Show cursor
                printf("\033[?25h");
                uiMode = UIMode::UIMODE_SHELL;
                log.setEnabled(false);
                shell.reset();
            } 
            else if (c == 'i') {
                txCtl0.forceId();
                txCtl1.forceId();
            }
        }

        // Running LED
        if (flash) {
            if (liveLED) 
                gpio_put(LED_PIN, 1);
            else
                gpio_put(LED_PIN, 0);
            liveLED = !liveLED;
        }

        // Transmit LED
        if (tx0.getPtt() || tx1.getPtt()) 
            gpio_put(LED2_PIN, 1);
        else
            gpio_put(LED2_PIN, 0);

        // Check for commands
        char d0 = core0.getLastDtmfDetection();
        if (d0 != 0) {
            log.info("DTMF [%c]", d0);
            dtmfCmdProc.processSymbol(d0);
        }
        char d1 = core1.getLastDtmfDetection();
        if (d1 != 0) {
            log.info("DTMF [%c]", d1);
            dtmfCmdProc.processSymbol(d1);
        }

        // Mute receivers when command processing is going on
        core0.setRxMute(dtmfCmdProc.isAccess());
        core1.setRxMute(dtmfCmdProc.isAccess());

        // ----- Adjust Receiver Routing/Mixing -----------------------------------
        //
        // This is an ongoing process of adjusting the "cross gains" of the 
        // transmitter to make sure the audio from the correct receivers is 
        // being mixed and passed through to this transmitter. 
        // 
        // This is a low-cost operation so, to simplify the logic, it is just
        // done all the time.
        unsigned activeCount = 0;
        if (rx0.isActive())
            activeCount++;
        if (rx1.isActive())
            activeCount++;
        if (core2.isActive())
            activeCount++;

        // Divide the gain evenly across the active receivers
        float gain = (activeCount != 0) ? 1.0 / (float)activeCount : 0;
        core0.setCrossGainLinear(0, rx0.isActive() ? gain : 0.0);
        core0.setCrossGainLinear(1, rx1.isActive() ? gain : 0.0);
        core0.setCrossGainLinear(2, core2.isActive() ? gain : 0.0);
        core1.setCrossGainLinear(0, rx0.isActive() ? gain : 0.0);
        core1.setCrossGainLinear(1, rx1.isActive() ? gain : 0.0);
        core1.setCrossGainLinear(2, core2.isActive() ? gain : 0.0);
        core2.setCrossGainLinear(0, rx0.isActive() ? gain : 0.0);
        core2.setCrossGainLinear(1, rx1.isActive() ? gain : 0.0);
        core2.setCrossGainLinear(2, core2.isActive() ? gain : 0.0);
   
        // Run all components
        tx0.run();
        tx1.run();
        rx0.run();
        rx1.run();
        txCtl0.run();
        txCtl1.run();
        dtmfCmdProc.run();

        uint32_t t = perfTimerLoop.elapsedUs();
        if (t > longestLoop)
            longestLoop = t;
    }
}
