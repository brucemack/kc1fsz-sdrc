#include <stdio.h>
#include <cassert>

#include "StdRx.h"

namespace kc1fsz {

StdRx::StdRx(Clock& clock, Log& log, int id, int cosPin, int tonePin,
    CourtesyToneGenerator::Type courtesyType, AudioCore& core) 
:   _clock(clock),
    _log(log),
    _id(id),
    // Flip logic because of the inverter in the hardware design
    _cosPin(cosPin, true),
    _tonePin(tonePin, true),
    _cosDebouncer(clock, _cosValue),
    _toneDebouncer(clock, _tonePin),
    _courtesyType(courtesyType),
    _core(core),
    _startTime(_clock.time()),
    _cosValue(cosPin, core) {
}

void StdRx::run() {
}

bool StdRx::isCOS() const {
    if (_cosMode == Rx::CosMode::COS_EXT_LOW || 
        _cosMode == Rx::CosMode::COS_EXT_HIGH) {
        return _cosDebouncer.get();
    } else {
        // TODO: NEED TO SUPPORT SOFT COS
        return false;
    }
}

bool StdRx::isCTCSS() const {
    if (_toneMode == ToneMode::TONE_EXT_LOW ||
        _toneMode == ToneMode::TONE_EXT_HIGH) {
        return _toneDebouncer.get();
    } else {
        // TODO: NEED TO SUPPORT SOFT TONE
        return false;
    }
}

bool StdRx::isActive() const { 
    return (_cosMode == CosMode::COS_IGNORE || isCOS()) && 
           (_toneMode == ToneMode::TONE_IGNORE || isCTCSS());
}

}
