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

namespace kc1fsz {

class AudioCoreOutputPort;

/**
 * A state machine that generates courtesy tones of a few different types.
 * This relies on the AudioCoreOutputPort methods for controlling audio
 * tones. 
 * 
 * To improve sound quality the on/off behavior is controlled using the 
 * .setToneLevel() method with the expectation that the audio implementation
 * will create a smooth transition and avoid "clicks."
 */
class CourtesyToneGenerator : public ToneGenerator {
public:

    enum Type { NONE, SINGLE, FAST_UPCHIRP, FAST_DOWNCHIRP };

    CourtesyToneGenerator(Log& log, Clock& clock, AudioCoreOutputPort& core);

    virtual void run();
    virtual void start();
    virtual bool isFinished();
    void setType(Type type) { _type = type; }
    void setLevel(float db) { _level = db; }

private:

    Log& _log;
    Clock& _clock;
    AudioCoreOutputPort& _core;

    unsigned int _chirpMs = 40;
    unsigned int _toneMs = 120;
    bool _running = false;
    Type _type = Type::FAST_UPCHIRP;
    float _level = -10;
    int _part = 0;
    uint32_t _endTime = 0;
};

}
