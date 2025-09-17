#include "Config.h"

#include <cstring>

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

#include "kc1fsz-tools/Common.h"

namespace kc1fsz {

void Config::saveConfig(const Config* cfg) {
    uint32_t ints = save_and_disable_interrupts();
    // Must erase a full sector first (4096 bytes)
    flash_range_erase((PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
    // IMPORTANT: Must be a multiple of 256!
    flash_range_program((PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE), (uint8_t*)cfg, 
        Config::CONFIG_SIZE);
    restore_interrupts(ints);
}

void Config::loadConfig(Config* cfg) {
    assert(sizeof(Config) == 512);
    // The very last sector of flash is used. Compute the memory-mapped address, 
    // remembering to include the offset for RAM
    const uint8_t* addr = (uint8_t*)(XIP_BASE + (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE));
    memcpy((void*)cfg, (const void*)addr, Config::CONFIG_SIZE);
}

void Config::setFactoryDefaults(Config* cfg) {

    cfg->magic = CONFIG_VERSION;

    // General
    strcpyLimited(cfg->general.callSign, "KC1FSZ", Config::callSignMaxLen);
    strcpyLimited(cfg->general.pass, "781", Config::passMaxLen);
    cfg->general.repeatMode = 2;
    cfg->general.diagFreq = 1000;
    cfg->general.diagLevel = -10;
    cfg->general.idRequiredInt = 10 * 60;

    // Receiver
    cfg->rx0.cosMode = 3;
    cfg->rx0.cosActiveTime = 25;
    cfg->rx0.cosInactiveTime = 250;
    // This is in dB!
    cfg->rx0.cosLevel = -40;
    // Soft
    cfg->rx0.toneMode = 0;
    cfg->rx0.toneActiveTime = 50;
    cfg->rx0.toneInactiveTime = 150;
    // This is in dB!
    cfg->rx0.toneLevel = -60;
    cfg->rx0.toneFreq = 123;
    // This is in dB!
    cfg->rx0.gain = 0;
    cfg->rx0.ctMode = 0;
    cfg->rx0.delayTime = 0;
    cfg->rx0.agcMode = 1;
    cfg->rx0.agcLevel = -10;
    cfg->rx0.dtmfDetectLevel = -50;
    cfg->rx0.deemphMode = 0;
    cfg->rx1 = cfg->rx0;

    cfg->rx0.cosMode = 0;
    cfg->rx0.toneMode = 3;

    // Transmitter
    cfg->tx0.enabled = false;
    cfg->tx0.toneMode = 0;
    // This is in dB!
    cfg->tx0.toneLevel = -16;
    cfg->tx0.toneFreq = 123;
    // This is in dB!
    cfg->tx0.gain = 0;
    cfg->tx0.enabled2 = true;

    cfg->tx1.enabled = false;
    cfg->tx1.toneMode = 0;
    // This is in dB!
    cfg->tx1.toneLevel = -16;
    cfg->tx1.toneFreq = 88.5;
    // This is in dB!
    cfg->tx1.gain = 0;
    cfg->tx1.enabled2 = true;

    // Controller
    cfg->txc0.timeoutTime = 120 * 1000;
    cfg->txc0.lockoutTime = 60 * 1000;
    cfg->txc0.hangTime = 1500;
    cfg->txc0.ctLevel = -10;
    cfg->txc0.idMode = 0;
    cfg->txc0.idLevel = -10;
    for (unsigned i = 0; i < Config::maxReceivers; i++)
        cfg->txc0.rxEligible[i] = false;
    cfg->txc0.rxEligible[0] = true;
    cfg->txc0.rxEligible[1] = true;
    cfg->txc1 = cfg->txc0;

    // Cross-band setup
    cfg->txc0.idMode = 1;
    cfg->txc0.rxEligible[0] = false;
    cfg->txc1.rxEligible[1] = false;
}

void Config::_showRx(const Config::ReceiveConfig* cfg,
    const char* pre) {
    printf("%s cosmode: %d\n", pre, cfg->cosMode);
    printf("%s cosactivetime: %d\n", pre, cfg->cosActiveTime);
    printf("%s cosinactivetime: %d\n", pre, cfg->cosInactiveTime);
    printf("%s coslevel: %.1f\n", pre, cfg->cosLevel);
    printf("%s rxtonemode: %d\n", pre, cfg->toneMode);
    printf("%s rxtoneactivetime: %d\n", pre, cfg->toneActiveTime);
    printf("%s rxtoneinactivetime: %d\n", pre, cfg->toneInactiveTime);
    printf("%s rxtonelevel: %.1f\n", pre, cfg->toneLevel);
    printf("%s rxtonefreq: %.1f\n", pre, cfg->toneFreq);
    printf("%s rxgain: %.1f\n", pre, cfg->gain);
    printf("%s ctmode: %d\n", pre, cfg->ctMode);
    printf("%s delaytime: %d\n", pre, cfg->delayTime);
    printf("%s agcmode: %d\n", pre, cfg->agcMode);
    printf("%s agclevel: %.1f\n", pre, cfg->agcLevel);
    printf("%s dtmfdetectlevel: %.1f\n", pre, cfg->dtmfDetectLevel);
    printf("%s deemphmode: %d\n", pre, cfg->deemphMode);
}

void Config::_showTx(const Config::TransmitConfig* cfg,
    const char* pre) {
    printf("%s txenable: %d\n", pre, cfg->enabled);
    printf("%s txenable2: %d\n", pre, cfg->enabled2);
    printf("%s txtonemode  : %d\n", pre, cfg->toneMode);
    printf("%s txtonelevel  : %.1f\n", pre, cfg->toneLevel);
    printf("%s txtonefreq  : %.1f\n", pre, cfg->toneFreq);
    printf("%s txgain  : %.1f\n", pre, cfg->gain);
}

void Config::_showTxc(const Config::ControlConfig* cfg,
    const char* pre) {
    printf("%s timeouttime  : %d\n", pre, cfg->timeoutTime);
    printf("%s lockouttime  : %d\n", pre, cfg->lockoutTime);
    printf("%s hangtime  : %d\n", pre, cfg->hangTime);
    printf("%s ctlevel  : %.1f\n", pre, cfg->ctLevel);
    printf("%s idmode: %d\n", pre, cfg->idMode);
    printf("%s idlevel: %.1f\n", pre, cfg->idLevel);
    printf("%s rxrepeat : ", pre);
    for (unsigned i = 0; i < Config::maxReceivers; i++)
        if (cfg->rxEligible[i]) 
            printf("%d ", i);
    printf("\n");
}

void Config::show(const Config* cfg) {
    // General configuration
    printf("   callsign      : %s\n", cfg->general.callSign);
    printf("   pass          : %s\n", cfg->general.pass);
    printf("   repeatmode    : %d\n", cfg->general.repeatMode);
    printf("   testtonefreq  : %.1f\n", cfg->general.diagFreq);
    printf("   testtonelevel : %.1f\n", cfg->general.diagLevel);
    printf("   idrequiredint : %u\n", cfg->general.idRequiredInt);
    printf("\nRadio 0\n");
    _showRx(&cfg->rx0, "R0");
    _showTx(&cfg->tx0, "T0");
    _showTxc(&cfg->txc0, "T0");
    printf("\nRadio 1\n");
    _showRx(&cfg->rx1, "R1");
    _showTx(&cfg->tx1, "T1");
    _showTxc(&cfg->txc1, "T1");
}

}
