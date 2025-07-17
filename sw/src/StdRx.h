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
 *
 * NOT FOR COMMERCIAL USE WITHOUT PERMISSION.
 */
#ifndef _StdRx_h
#define _StdRx_h

#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/Clock.h"
#include "kc1fsz-tools/BinaryWrapper.h"
#include "kc1fsz-tools/rp2040/GpioValue.h"
#include "kc1fsz-tools/TimeDebouncer.h"

#include "Rx.h"
#include "AudioCore.h"

namespace kc1fsz {

/**
 * @brief A utility class that wraps a hardware signal and
 * the AudioCore to produce a unified COS indicator, dependent
 * on the COS mode selected.
 */
class COSValue : public BinaryWrapper {
public:

    COSValue(BinaryWrapper& hwValue, AudioCore& core)
    :   _hwValue(hwValue),
        _core(core)
    {
    }

    bool get() const { 
        if (_useHw)
            return _hwValue.get();
        else 
            return _core.getSignalRms() > _thresholdRms;
    }

    void setUseHw(bool b) { _useHw = b; }
    void setThresholdRms(float rms) { _thresholdRms = rms; }

private:

    BinaryWrapper& _hwValue;
    AudioCore& _core;
    bool _useHw = true;
    float _thresholdRms = 0.1;
};

class StdRx : public Rx {
public:

    StdRx(Clock& clock, Log& log, int id, int cosPin, int tonePin, 
        CourtesyToneGenerator::Type courtesyType, AudioCore& core);

    virtual int getId() const { return _id; }
    virtual void run();

    virtual bool isActive() const;
    virtual bool isCOS() const;
    virtual bool isCTCSS() const;

    void setCosMode(CosMode mode) { 
        _cosMode = mode; 
        _cosPin.setActiveLow(mode == Rx::CosMode::COS_EXT_LOW);
        _cosValue.setUseHw(mode == Rx::CosMode::COS_EXT_LOW || 
            mode == Rx::CosMode::COS_EXT_HIGH);
    }

    void setCosActiveTime(unsigned ms) { _cosDebouncer.setActiveTime(ms); }

    void setCosInactiveTime(unsigned ms) { _cosDebouncer.setInactiveTime(ms); }

    void setCosLevel(float db) { 
        // Send the level down to the COSValue object that actually
        // performs the comparison.
        _cosValue.setThresholdRms(pow(10, (db / 20)));
    }

    void setToneMode(ToneMode mode) { 
        _toneMode = mode; 
        _tonePin.setActiveLow(mode == Rx::ToneMode::TONE_EXT_LOW);
    }

    void setToneActiveTime(unsigned ms) { 
        _toneDebouncer.setActiveTime(ms); 
    }

    void setToneInactiveTime(unsigned ms) { 
        _toneDebouncer.setInactiveTime(ms); 
    }

    void setToneLevel(float db) { _toneLevel = db; }
    void setToneFreq(float hz) { _core.setCtcssDecodeFreq(hz); }
    void setGain(float lvl) { _gain = lvl; }

    virtual CourtesyToneGenerator::Type getCourtesyType() const { 
        return _courtesyType;
    }

private:

    Clock& _clock;
    Log& _log;
    const int _id;
    GpioValue _cosPin;
    GpioValue _tonePin;
    AudioCore& _core;

    // This combines the _cosPin and _core information to created 
    // a COS indicator that is ready to be debounced.
    COSValue _cosValue;

    TimeDebouncer _cosDebouncer;
    TimeDebouncer _toneDebouncer;

    const CourtesyToneGenerator::Type _courtesyType;

    uint32_t _startTime;
    bool _active = false;
    unsigned int _state = 0;

    CosMode _cosMode = CosMode::COS_EXT_HIGH;

    ToneMode _toneMode = ToneMode::TONE_IGNORE;
    float _toneLevel = -26;

    float _gain = 1.0;
};

}

#endif
