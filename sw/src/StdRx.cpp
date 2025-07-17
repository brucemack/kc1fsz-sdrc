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
    _toneDebouncer(clock, _toneValue),
    _courtesyType(courtesyType),
    _core(core),
    _startTime(_clock.time()),
    _cosValue(_cosPin, core),
    _toneValue(_tonePin, core) {
}

void StdRx::run() {
}

bool StdRx::isCOS() const {
    return _cosDebouncer.get();
}

bool StdRx::isCTCSS() const {
    return _toneDebouncer.get();
}

bool StdRx::isActive() const { 
    return (_cosMode == CosMode::COS_IGNORE || isCOS()) && 
           (_toneMode == ToneMode::TONE_IGNORE || isCTCSS());
}

}
