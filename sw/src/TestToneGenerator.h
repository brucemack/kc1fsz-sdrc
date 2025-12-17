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
#pragma once

#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/Clock.h"

#include "ToneGenerator.h"
#include "AudioCoreOutputPort.h"

namespace kc1fsz {

class TestToneGenerator : public ToneGenerator {
public:

    TestToneGenerator(Log& log, Clock& clock, AudioCoreOutputPort& core);

    virtual void run();

    virtual void start();
    virtual void stop();
    virtual bool isFinished();
    void setFreq(float hz) { _freq = hz; _core.setToneFreq(_freq); }
    void setLevel(float db) { _level = db; _core.setToneLevel(_level); }

private:

    Log& _log;
    Clock& _clock;
    AudioCoreOutputPort& _core;

    float _freq = 1000.0;
    float _level = -10.0;    
    bool _running = false;
    uint32_t _endTime = 0;
};

}
