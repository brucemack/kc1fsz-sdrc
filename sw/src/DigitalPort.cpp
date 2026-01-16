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
//
// This is called when a packet of audio is received from the network. The 
// data is placed in the circular buffer so it is available for cycleRx() on
// the next audio tick.
//
void DigitalPort::loadNetworkAudio(const uint8_t* audio8KLE, unsigned len) {
    assert(len == NETWORK_FRAME_SIZE);
    // Move new data into circular buffer
    for (unsigned i = 0; i < len; i++) {
        _extAudioIn[_extAudioInWr++] = audio8KLE[i];
        if (_extAudioInWr == _extAudioInCapacity)
            _extAudioInWr = 0;
        _extAudioInLen++;
    }
    _lastInputUs = _clock.timeUs();
}

// ****************************************************************************
// NOTE: This function is called from inside of the audio frame ISR so keep it 
// short!
// ****************************************************************************
// 
// This is called on each tick to extract a frame of audio for playback.
//
void DigitalPort::cycleRx(float* crossOut) {    
    // Check for the drain situation.
    if (_extAudioInLen < BLOCK_SIZE * 2) {
        for (unsigned i = 0; i < BLOCK_SIZE; i++)
            crossOut[i] = 0;       
        _extAudioInTriggered = false; 
    }
    else {
        // Decide if we have enough of a backlog to start delivering the audio.
        // This is a very simple jitter buffer mechanism to avoid problems with
        // subtle timing differences between network arrival and the 
        // audio tick rate. It's better to keep the playout slightly behind the 
        // network arrival cadence. 
        if (_extAudioInLen >= BLOCK_SIZE * 2) {
            _extAudioInTriggered = true;
        }

        if (_extAudioInTriggered) {
            for (unsigned i = 0; i < BLOCK_SIZE; i++)  {
                crossOut[i] = 
                    (float)unpack_int16_le(_extAudioIn + _extAudioInRd) / 32767.0f;
                // Increment and deal with wrap
                _extAudioInRd += 2;
                if (_extAudioInRd == _extAudioInCapacity)
                    _extAudioInRd = 0;
                _extAudioInLen -= 2;
            }
        } else {
            for (unsigned i = 0; i < BLOCK_SIZE; i++)
                crossOut[i] = 0;       
        }
    }
}

// ****************************************************************************
// NOTE: This function is called from inside of the audio frame ISR so keep it 
// short!
// ****************************************************************************
// 
// This function is called on every audio tick. It delivers the output
// audio that should be sent out on the network as soon as possible.
//
void DigitalPort::cycleTx(const float** cross_ins) {
    // Mix all of the audio sources and produce a single 8K PCM16 frame
    for (unsigned i = 0; i < BLOCK_SIZE; i++)  {
        float mix = 0;
        for (unsigned j = 0; j < _crossCount; j++)
            mix += _crossGains[j] * cross_ins[j][i];
        int16_t pcm = mix * 32767.0f;
        pack_int16_le(pcm, _extAudioOut + _extAudioOutWr);
        // Increment and deal with wrap
        _extAudioOutWr += 2;
        if (_extAudioOutWr == _extAudioOutCapacity)
            _extAudioOutWr = 0;
    }
}

// ****************************************************************************
// NOTE: This function is called from inside of the audio frame ISR so keep it 
// short!
// ****************************************************************************
void DigitalPort::extractNetworkAudio(uint8_t* audio8KLE, unsigned len) {
    assert(len == NETWORK_FRAME_SIZE);
    // Move new data out of circular buffer
    for (unsigned i = 0; i < len; i++) {
        // Only consume data if it is available
        if (_extAudioOutRd == _extAudioOutWr)
            audio8KLE[i] = 0;
        else {
            audio8KLE[i] = _extAudioOut[_extAudioOutRd++];
            if (_extAudioOutRd == _extAudioOutCapacity)
                _extAudioOutRd = 0;
        }
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

}
