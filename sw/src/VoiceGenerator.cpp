#include "VoiceGenerator.h"

#include "audio.h"

namespace kc1fsz {

VoiceGenerator::VoiceGenerator(Log& log, Clock& clock, ToneSynthesizer& synth) 
:   _log(log),
    _clock(clock),
    _synth(synth)
{
}

void VoiceGenerator::run() {
    if (_running) {
        if (_clock.isPast(_endTime))  {            
            _log.info("Voice ID end");
            _running = false;
        }
    }
}

void VoiceGenerator::start() {
    _running = true;
    // Calculate the length of time that the voice will run
    float fs = 8000;
    float timeMs = (audio_id_length / fs) * 1000.0;
    _endTime = _clock.time() + (unsigned int)timeMs;
    // Trigger voice
    _synth.setPcm(audio_id, audio_id_length, 16000);
    _synth.setEnabled(true);
    _log.info("Voice ID start");
}

bool VoiceGenerator::isFinished() {
    return !_running;
}

}
