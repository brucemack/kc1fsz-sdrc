#ifndef _VoiceGenerator_h
#define _VoiceGenerator_h

#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/Clock.h"

#include "ToneGenerator.h"

namespace kc1fsz {

class AudioCore;

class VoiceGenerator : public ToneGenerator {
public:

    VoiceGenerator(Log& log, Clock& clock, AudioCore& core);

    virtual void run();
    virtual void start();
    virtual bool isFinished();

private:

    Log& _log;
    Clock& _clock;
    AudioCore& _core;
    
    bool _running = false;
    uint32_t _endTime = 0;
};

}

#endif
