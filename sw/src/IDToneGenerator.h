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
#ifndef _IDToneGenerator_h
#define _IDToneGenerator_h

#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/Clock.h"

#include "ToneGenerator.h"

namespace kc1fsz {

class AudioCore;

class IDToneGenerator : public ToneGenerator {
public:

    IDToneGenerator(Log& log, Clock& clock, AudioCore& core);

    virtual void run();

    virtual void start();
    virtual bool isFinished();

    void setCall(const char* callSign);
    void setLevel(float db) { _level = db; }

private:

    Log& _log;
    Clock& _clock;
    AudioCore& _core;

    static const unsigned _maxCallSignLen = 16;
    char _callSign[_maxCallSignLen];
    float _level;
    
    bool _running = false;
    uint32_t _endTime = 0;
    unsigned int _state = 0;
    unsigned int _callPtr = 0;
    unsigned int _symPtr = 0;
};

}

#endif
