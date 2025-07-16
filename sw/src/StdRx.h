#ifndef _StdRx_h
#define _StdRx_h

#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/Clock.h"
#include "kc1fsz-tools/rp2040/GpioValue.h"
#include "kc1fsz-tools/TimeDebouncer.h"

#include "Rx.h"
#include "AudioCore.h"

namespace kc1fsz {

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
    }

    void setCosActiveTime(unsigned ms) { _cosDebouncer.setActiveTime(ms); }
    void setCosInactiveTime(unsigned ms) { _cosDebouncer.setInactiveTime(ms); }
    void setCosLevel(float lvl) { _cosLevel = lvl; }

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

    TimeDebouncer _cosDebouncer;
    TimeDebouncer _toneDebouncer;

    const CourtesyToneGenerator::Type _courtesyType;

    uint32_t _startTime;
    bool _active = false;
    unsigned int _state = 0;

    CosMode _cosMode = CosMode::COS_EXT_HIGH;
    float _cosLevel = 0.0;

    ToneMode _toneMode = ToneMode::TONE_IGNORE;
    float _toneLevel = -26;

    float _gain = 1.0;
};

}

#endif
