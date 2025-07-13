#include "TestTx.h"

namespace kc1fsz {

TestTx::TestTx(Clock& clock, Log& log, int id) 
:   _clock(clock),
    _log(log),
    _id(id) {
}

void TestTx::setPtt(bool ptt) {
    if (ptt != _keyed)
        if (ptt) {
            if (_toneX10)
                _log.info("Tone on (x10) %d [%d]", _toneX10, _id);
            _log.info("Transmitter keyed [%d]", _id);
        } else {
            _log.info("Transmitter unkeyed [%d]", _id);
            if (_toneX10)
                _log.info("Tone off [%d]", _id);
        }
    _keyed = ptt;
}

void TestTx::run() {   
}

}