#include <stdio.h>
#include "TestRx.h"

namespace kc1fsz {

TestRx::TestRx(Clock& clock, Log& log, int id) 
:   _clock(clock),
    _log(log),
    _id(id) {
    _startTime = _clock.time();
    _active = false;
}

void TestRx::run() {
    // Simulate a quick kerchunck
    if (_state == 0) {
        if (_clock.isPast(_startTime + (5 * 1000))) {
            _active = true; 
            _state = 1;
            _log.info("TEST 000: Simulating receiver kerchunk [%d]", _id);
        }
    }
    else if (_state == 1) {
        if (_clock.isPast(_startTime + (7 * 1000))) {
            _active = false; 
            _state = 200;
        }
    }
    // Simulate a quick kerchunck
    if (_state == 200) {
        if (_clock.isPast(_startTime + (20 * 1000))) {
            _active = true; 
            _state = 201;
            _log.info("TEST 001: Simulating receiver kerchunk [%d]", _id);
        }
    }
    else if (_state == 201) {
        if (_clock.isPast(_startTime + (21 * 1000))) {
            _active = false; 
            _state = 2;
        }
    }
    // Simulate a long conversation (125 seconds)
    else if (_state == 2) {
        if (_clock.isPast(_startTime + (40 * 1000))) {
            _active = true; 
            _state = 3;
            _log.info("TEST 002: Simulating receiver transmission for 125 seconds [%d]", _id);
        }
    }
    else if (_state == 3) {
        if (_clock.isPast(_startTime + (165 * 1000))) {
            _active = false; 
            _state = 4;
        }
    }
    // Simulate a quick kerchunck inside of the lockout
    else if (_state == 4) {
        if (_clock.isPast(_startTime + (170 * 1000))) {
            _active = true; 
            _state = 5;
            _log.info("TEST 003: Simulating receiver kerchunk inside of lockout (will be ignored) [%d]", _id);
        }
    }
    else if (_state == 5) {
        if (_clock.isPast(_startTime + (172 * 1000))) {
            _active = false; 
            _state = 6;
        }
    }
    // Simulate a quick kerchunck after lockout
    else if (_state == 6) {
        if (_clock.isPast(_startTime + (305 * 1000))) {
            _active = true; 
            _state = 7;
            _log.info("TEST 004: Simulating receiver kerchunk after lockout (should work normally now) [%d]", _id);
        }
    }
    else if (_state == 7) {
        if (_clock.isPast(_startTime + (307 * 1000))) {
            _active = false; 
            _state = 8;
        }
    }
    // Simulate a call with a short-dropout
    if (_state == 8) {
        if (_clock.isPast(_startTime + (345 * 1000))) {
            _active = true; 
            _state = 9;
            _log.info("TEST 005: Simulating receiver transmission with short (200ms) dropout [%d]", _id);
        }
    }
    else if (_state == 9) {
        if (_clock.isPast(_startTime + (347000))) {
            _active = false; 
            _state = 10;
        }
    }
    else if (_state == 10) {
        if (_clock.isPast(_startTime + (347200))) {
            _active = true; 
            _state = 11;
        }
    }
    else if (_state == 11) {
        if (_clock.isPast(_startTime  + (349 * 1000))) {
            _active = false; 
            _state = 12;
        }
    }
    // Simulate two calls
    else if (_state == 12) {
        if (_clock.isPast(_startTime + (400 * 1000))) {
            _active = true; 
            _state = 13;
            _log.info("TEST 006: Simulating two transmissions, with the second inside of the hang [%d]", _id);
        }
    }
    else if (_state == 13) {
        if (_clock.isPast(_startTime + (410 * 1000))) {
            _active = false; 
            _state = 14;
        }
    }
    if (_state == 14) {
        if (_clock.isPast(_startTime + (412 * 1000))) {
            _active = true; 
            _state = 15;
        }
    }
    else if (_state == 15) {
        if (_clock.isPast(_startTime + (422 * 1000))) {
            _active = false; 
            _state = 16;
        }
    }
}

}
