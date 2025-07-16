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
    strcpyLimited(cfg->general.callSign, "W1TKZ", Config::callSignMaxLen);
    strcpyLimited(cfg->general.pass, "781", Config::passMaxLen);
    cfg->general.repeatMode = 2;
    cfg->general.diagMode = 0;
    cfg->general.diagFreq = 1000;
    cfg->general.diagLevel = -10;

    // Receiver
    cfg->rx0.cosMode = 2;
    cfg->rx0.cosActiveTime = 25;
    cfg->rx0.cosInactiveTime = 25;
    cfg->rx0.cosLevel = -20;
    cfg->rx0.toneMode = 2;
    cfg->rx0.toneActiveTime = 25;
    cfg->rx0.toneInactiveTime = 25;
    cfg->rx0.toneLevel = -10;
    cfg->rx0.toneFreq = 700;
    cfg->rx0.gain = 0;
    cfg->rx1 = cfg->rx0;

    // Transmitter
    cfg->tx0.toneMode = 0;
    cfg->tx0.toneFreq = 0;
    cfg->tx0.toneLevel = -16;
    cfg->tx0.gain = 1.0;
    cfg->tx1.toneMode = 1;
    cfg->tx1.toneFreq = 88.5;
    cfg->tx1.toneLevel = -16;
    cfg->tx1.gain = 1.0;

    // Controller
    cfg->txc0.timeoutTime = 120 * 1000;
    cfg->txc0.lockoutTime = 60 * 1000;
    cfg->txc0.hangTime = 1500;
    cfg->txc0.ctMode = 2;
    cfg->txc0.ctLevel = -10;
    cfg->txc0.idLevel = -10;
    cfg->txc1 = cfg->txc0;

}

void Config::_showRx(const Config::ReceiveConfig* cfg,
    const char* pre) {
    printf("%s cosmode  : %d\n", pre, cfg->cosMode);
    printf("%s cosactivetime  : %d\n", pre, cfg->cosActiveTime);
    printf("%s cosinactivetime  : %d\n", pre, cfg->cosInactiveTime);
    printf("%s coslevel  : %.1f\n", pre, cfg->cosLevel);
    printf("%s tonemode  : %d\n", pre, cfg->toneMode);
    printf("%s toneactivetime  : %d\n", pre, cfg->toneActiveTime);
    printf("%s toneinactivetime  : %d\n", pre, cfg->toneInactiveTime);
    printf("%s tonelevel  : %.1f\n", pre, cfg->toneLevel);
    printf("%s tonefreq  : %.1f\n", pre, cfg->toneFreq);
    printf("%s gain  : %.1f\n", pre, cfg->gain);
}

void Config::_showTx(const Config::TransmitConfig* cfg,
    const char* pre) {
    printf("%s tonemode  : %d\n", pre, cfg->toneMode);
    printf("%s tonelevel  : %.1f\n", pre, cfg->toneLevel);
    printf("%s tonefreq  : %.1f\n", pre, cfg->toneFreq);
    printf("%s gain  : %.1f\n", pre, cfg->gain);
}

void Config::_showTxc(const Config::ControlConfig* cfg,
    const char* pre) {
    printf("%s timeouttime  : %d\n", pre, cfg->timeoutTime);
    printf("%s lockouttime  : %d\n", pre, cfg->lockoutTime);
    printf("%s hangtime  : %d\n", pre, cfg->hangTime);
    printf("%s ctmode  : %d\n", pre, cfg->ctMode);
    printf("%s ctlevel  : %.1f\n", pre, cfg->ctLevel);
    printf("%s idlevel  : %.1f\n", pre, cfg->idLevel);
}

void Config::show(const Config* cfg) {
    // General configuration
    printf("   callsign      : %s\n", cfg->general.callSign);
    printf("   pass          : %s\n", cfg->general.pass);
    printf("   repeatmode    : %d\n", cfg->general.repeatMode);
    printf("   testmode      : %d\n", cfg->general.diagMode);
    printf("   testtonefreq  : %.1f\n", cfg->general.diagFreq);
    printf("   testtonelevel : %.1f\n", cfg->general.diagLevel);
    // Receiver configuration
    _showRx(&cfg->rx0, "R0");
    _showRx(&cfg->rx1, "R1");
    // Transmitter configuration
    _showTx(&cfg->tx0, "T0");
    _showTxc(&cfg->txc0, "T0");
    _showTx(&cfg->tx1, "T1");
    _showTxc(&cfg->txc1, "T1");
}

}
