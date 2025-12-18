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
#include <cassert>

#include "CourtesyToneGenerator.h"
#include "AudioCoreOutputPort.h"

namespace kc1fsz {

CourtesyToneGenerator::CourtesyToneGenerator(Log& log, Clock& clock, AudioCoreOutputPort& core) 
:   _log(log),
    _clock(clock),
    _core(core)
{
}

void CourtesyToneGenerator::run() {
    if (_running) {
        if (_clock.isPast(_endTime)) {
            if (_part == 0) {
                if (_type == Type::FAST_DOWNCHIRP)
                    _core.setToneFreq(1000);
                else if (_type == Type::FAST_UPCHIRP)
                    _core.setToneFreq(1280);
                _part = 1;
                _endTime = _clock.time() + _chirpMs;
            } else if (_part == 1) {
                _core.setToneLevel(-96);
                _part = 2;
                _endTime = _clock.time() + _toneMs;
            } else if (_part == 2) {
                _core.setToneEnabled(false);
                _running = false;
            } else {
                assert(false);
            }
        }
    }
}

void CourtesyToneGenerator::start() {
    if (_type == Type::NONE) {
        _running = false;
    } 
    else {
        _running = true;
        _core.setToneEnabled(true);
        _core.setToneLevel(_level);
        if (_type == Type::SINGLE) {
            _part = 1;
            _endTime = _clock.time() + _toneMs;
            _core.setToneFreq(800);
        }
        else if (_type == Type::FAST_DOWNCHIRP) {
            _part = 0;
            _endTime = _clock.time() + _chirpMs;
            _core.setToneFreq(1280);
        }
        else if (_type == Type::FAST_UPCHIRP) {
            _part = 0;
            _endTime = _clock.time() + _chirpMs;
            _core.setToneFreq(1000);
        }
    }
}

bool CourtesyToneGenerator::isFinished() {
    return !_running;
}

}
