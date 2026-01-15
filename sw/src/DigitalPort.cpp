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
}

// ****************************************************************************
// NOTE: This function is called from inside of the audio frame ISR so keep it 
// short!
// ****************************************************************************
void DigitalPort::cycleRx(float* crossOut) {    
    const uint8_t* p = _extAudioIn;
    for (unsigned i = 0; i < BLOCK_SIZE; i++, p += 2)  {
        if (_extAudioInValid)
            crossOut[i] = (float)unpack_int16_le(p) / 32767.0f;
        else
            crossOut[i] = 0;
    }
    _extAudioInValid = false;
}

// ****************************************************************************
// NOTE: This function is called from inside of the audio frame ISR so keep it 
// short!
// ****************************************************************************
void DigitalPort::cycleTx(const float** cross_ins) {
    // Mix all of the audio sources and produce a single 8K PCM16 frame
    uint8_t* p = _extAudioOut;
    for (unsigned i = 0; i < BLOCK_SIZE; i++, p += 2)  {
        float mix = 0;
        for (unsigned j = 0; j < _crossCount; j++)
            mix += _crossGains[j] * cross_ins[j][i];
        int16_t pcm = mix * 32767.0f;
        pack_int16_le(pcm, p);
    }
}

void DigitalPort::setCrossGainLinear(unsigned i, float gain) {  
    assert(i < MAX_CROSS_COUNT);
    _crossGains[i] = gain;
}

bool DigitalPort::isActive() const {
    // If audio was received within the last 40ms then we are active
    return (_clock.timeUs() - _lastInputUs < 40 * 1000);
}

// ****************************************************************************
// NOTE: This function is called from inside of the audio frame ISR so keep it 
// short!
// ****************************************************************************
void DigitalPort::setAudio(const uint8_t* audio8KLE, unsigned len) {
    assert(len == BLOCK_SIZE * 2);
    memcpy(_extAudioIn, audio8KLE, len);
    _extAudioInValid = true;
    _lastInputUs = _clock.timeUs();
}

// ****************************************************************************
// NOTE: This function is called from inside of the audio frame ISR so keep it 
// short!
// ****************************************************************************
void DigitalPort::getAudio(uint8_t* audio8KLE, unsigned len) {
    assert(len == BLOCK_SIZE * 2);
    memcpy(audio8KLE, _extAudioOut, len);
}

}
