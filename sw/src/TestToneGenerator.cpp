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
#include "TestToneGenerator.h"
#include "AudioCore.h"

#define TEST_TONE_DUR_MS (30 * 1000)

namespace kc1fsz {

TestToneGenerator::TestToneGenerator(Log& log, Clock& clock, AudioCore& core) 
:   _log(log),
    _clock(clock),
    _core(core)
{
}

void TestToneGenerator::run() {
    if (_running) {
        if (_clock.isPast(_endTime)) {
            _running = false;
            _core.setToneEnabled(false);
            _log.info("Test tone end");
        }
    }
}

void TestToneGenerator::start() {
    _running = true;
    _endTime = _clock.time() + TEST_TONE_DUR_MS;
    _core.setToneFreq(_freq);
    _core.setToneLevel(_level);
    _core.setToneEnabled(true);
    _log.info("Test tone start");
}

void TestToneGenerator::stop() {
    _running = false;
    _core.setToneEnabled(false);
}

bool TestToneGenerator::isFinished() {
    return !_running;
}

}
