/**
 * Digital Repeater Controller
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
#include <cstdio>
#include <cassert>

#include "StdRx.h"

namespace kc1fsz {

StdRx::StdRx(Clock& clock, Log& log, int id, int cosPin, int tonePin, AudioCore& core) 
:   _clock(clock),
    _log(log),
    _id(id),
    // Flip logic because of the inverter in the hardware design
    _cosPin(cosPin, true),
    _tonePin(tonePin, true),
    _cosDebouncer(clock, _cosValue),
    _toneDebouncer(clock, _toneValue),
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
