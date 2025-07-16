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
#include <hardware/gpio.h>

#include "StdTx.h"

namespace kc1fsz {

StdTx::StdTx(Clock& clock, Log& log, int id, int pttPin, AudioCore& core)
:   _clock(clock),
    _log(log),
    _id(id),
    _pttPin(pttPin),
    _core(core) {
}

void StdTx::setPtt(bool ptt) {
    if (ptt != _keyed)
        if (ptt) {
            gpio_put(_pttPin, 1);
            _log.info("Transmitter keyed [%d]", _id);
        } else {
            gpio_put(_pttPin, 0);
            _log.info("Transmitter unkeyed [%d]", _id);
        }
    _keyed = ptt;
}

bool StdTx::getPtt() const {
    return _keyed;
}

void StdTx::run() {   
}

}