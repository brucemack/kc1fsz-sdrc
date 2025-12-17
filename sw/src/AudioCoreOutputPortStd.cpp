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
 */
#include "Rx.h"
#include "AudioCore.h"
#include "AudioCoreOutputPortStd.h"

namespace kc1fsz {

AudioCoreOutputPortStd::AudioCoreOutputPortStd(AudioCore& core, Rx& rx0, Rx& rx1)
:   _core(core), _rx0(rx0), _rx1(rx1) { 
}

bool AudioCoreOutputPortStd::isAudioActive() const {    
    return _rx0.isActive() || _rx1.isActive();
}

void AudioCoreOutputPortStd::setToneEnabled(bool b) {
    _core.setToneEnabled(b);
}

void AudioCoreOutputPortStd::setToneFreq(float hz) {
    _core.setToneFreq(hz);
}

void AudioCoreOutputPortStd::setToneLevel(float dbv) {
    _core.setToneLevel(dbv);
}

void AudioCoreOutputPortStd::resetDelay() {
    _core.resetDelay();
}

}
