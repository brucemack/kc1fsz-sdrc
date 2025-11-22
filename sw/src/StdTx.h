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
#ifndef _StdTx_h
#define _StdTx_h

#include <functional>

#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/Clock.h"

#include "Tx.h"
#include "AudioCore.h"

namespace kc1fsz {

class AudioCore;

class StdTx : public Tx {
public:

    StdTx(Clock& clock, Log& log, int id, int pttPin, AudioCore& core,
        std::function<bool()> positiveEnableCheck);

    virtual void run();
    virtual int getId() const { return _id; };

    virtual void setEnabled(bool en);
    virtual bool getEnabled() const;
    virtual void setPtt(bool ptt);
    virtual bool getPtt() const;

    void setToneMode(ToneMode mode) { 
        _toneMode = mode; 
        if (_toneMode == ToneMode::SOFT) {
            _core.setCtcssEncodeEnabled(true);
        } else {
            _core.setCtcssEncodeEnabled(false);
        }
    }

    void setToneFreq(float hz) { _core.setCtcssEncodeFreq(hz); }
    void setToneLevel(float db) { _core.setCtcssEncodeLevel(db); }

private:

    Clock& _clock;
    Log& _log;
    const int _id;
    const int _pttPin;
    AudioCore& _core;
    std::function<bool()> _positiveEnableCheck;

    bool _enabled = false;
    bool _keyed = false;

    // Configuration
    ToneMode _toneMode = ToneMode::NONE;
};

}

#endif
