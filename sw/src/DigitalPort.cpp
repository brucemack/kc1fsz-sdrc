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
 */
#include <cmath>
#include <cstring>
#include <cassert>

#include "DigitalPort.h"

#include "kc1fsz-tools/Clock.h"
#include "kc1fsz-tools/Common.h"

namespace kc1fsz {

DigitalPort::DigitalPort(unsigned id, unsigned crossCount, Clock& clock)
:   _id(id),
    _crossCount(crossCount),
    _clock(clock) {
    _omega = 2.0f * 3.1415926f * 440.0f / (float)FS;
    _phi = 0;
}

void DigitalPort::cycleRx(float* crossOut) {    
    /*
    // Contribute a steady tone
    for (unsigned i = 0; i < BLOCK_SIZE; i++) {
        crossOut[i] = 0.5 * std::cos(_phi);
        _phi += _omega;
    }
    _phi = fmod(_phi, 2.0 * 3.1415926f);
    */
    const uint8_t* p = _extAudio;
    for (unsigned i = 0; i < BLOCK_SIZE; i++, p += 2)  {
        if (_extAudioValid)
            crossOut[i] = (float)unpack_int16_le(p) / 32767.0f;
        else
            crossOut[i] = 0;
    }
    _extAudioValid = false;
}

void DigitalPort::cycleTx(const float** cross_ins) {
}

void DigitalPort::setCrossGainLinear(unsigned i, float gain) {  
}

bool DigitalPort::isActive() const {
    // Alternate seconds
    if ((_clock.time() / 1000) % 10 < 2)
        return true;
    else 
        return false;
}

void DigitalPort::setAudio(const uint8_t* audio8KLE, unsigned len) {
    assert(len == BLOCK_SIZE * 2);
    memcpy(_extAudio, audio8KLE, len);
    _extAudioValid = true;
}


}
