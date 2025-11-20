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
#include <iostream>

#include <kc1fsz-tools/Log.h>
#include <kc1fsz-tools/Clock.h>

#include "CommandProcessor.h"

using namespace std;

namespace kc1fsz {

CommandProcessor::CommandProcessor(Log& log, Clock& clock)
:   _log(log), 
    _clock(clock) { 
    _unlockCode[0] = '7';
    _unlockCode[1] = '8';
    _unlockCode[2] = '1';
    _unlockCode[3] = 0;
}

void CommandProcessor::processSymbol(char symbol) {

    if (symbol == ' ')
        return;

    _lastSymbolTime = _clock.time();

    // Kick the run function to address timeouts, etc.
    run();

    if (_access) {
        if (symbol == '*')
            return;
        // Put symbol onto queue
        if (_queueLen < MAX_QUEUE_LEN) {
            _queue[_queueLen++] = symbol;
        }
        _processQueue();
    }
    else if (symbol == '*') {
        if (_accessTrigger) _accessTrigger();
        _access = true;
    }
}

void CommandProcessor::run() {
    // After some amount of inactivity
    if (_access && _clock.isPast(_lastSymbolTime + _accessTimeout)) {
        _log.info("Access timed out");
        _queueLen = 0;
        _access = false;
    }
}

void CommandProcessor::_processQueue() {

    // Look for unlock code
    if (_queueEq(_unlockCode)) {
        _popQueue(strlen(_unlockCode));
        _unlock = true;
        _unlockUntil = _clock.time() + _unlockWindowMs;
        _log.info("Unlocked");
    }
    // Repeater system off
    else if (_queueEq("0")) {
        _popQueue(4);
        if (_unlock) {
            _notifyOk();
            if (_disableTrigger) _disableTrigger();
        }
    }
    // Repeater system on
    else if (_queueEq("1")) {
        _popQueue(4);
        if (_unlock) {
            _notifyOk();
            if (_reenableTrigger) _reenableTrigger();
        }
    }
    // Force ID/status
    else if (_queueEq("2")) {
        _popQueue(4);
        if (_unlock) {
            _notifyOk();
            if (_forceIdTrigger) _forceIdTrigger();
        }
    }
}

bool CommandProcessor::_queueEq(const char* a) const {
    if (_queueLen < strlen(a))
        return false;
    for (unsigned i = 0; i < _queueLen && i < strlen(a); i++)
        if (a[i] != _queue[i] && a[i] != '?')
            return false;
    return true;
}

void CommandProcessor::_popQueue(unsigned count) {
    for (unsigned i = 0; i < _queueLen - count; i++)
        _queue[i] = _queue[i + count];
    _queueLen -= count;
}

void CommandProcessor::_notifyOk() {
}

}