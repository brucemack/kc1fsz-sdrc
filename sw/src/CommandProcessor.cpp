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
    _unlockCode[2] = 0;
}

void CommandProcessor::processSymbol(char symbol) {

    if (symbol == ' ')
        return;

    _lastSymbolTime = _clock.time();

    // Kick the run function to address timeouts, etc.
    run();

    if (_access) {
        if (symbol == '*') {
            _queueLen = 0;
            // No change to access status
            return;
        }
        // Put symbol onto queue
        if (_queueLen < MAX_QUEUE_LEN) {
            _queue[_queueLen++] = symbol;
        }
        _processQueue();
    } else if (symbol == '*') {
        _enterAccess();
    }
}

void CommandProcessor::run() {
    // After some amount of inactivity
    if (_access && _clock.isPast(_lastSymbolTime + _accessTimeout)) {
        _log.info("Access timed out");
        _queueLen = 0;
        _exitAccess();
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
        _popQueue(3);
        if (_unlock) {
            _notifyOk();
            if (_disableTrigger) _disableTrigger();
        }
    }
    // Repeater system on
    else if (_queueEq("1")) {
        _popQueue(3);
        if (_unlock) {
            _notifyOk();
            if (_reenableTrigger) _reenableTrigger();
        }
    }
    // Force ID/status
    else if (_queueEq("2")) {
        _popQueue(3);
        if (_unlock) {
            // This command kicks us out of access mode
            _exitAccess();
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
    if (count > _queueLen)
        count = _queueLen;
    for (unsigned i = 0; i < _queueLen - count; i++)
        _queue[i] = _queue[i + count];
    _queueLen -= count;
}

void CommandProcessor::_notifyOk() {
}

void CommandProcessor::_enterAccess() {
    _access = true;
    if (_accessTrigger) _accessTrigger(_access);
}

void CommandProcessor::_exitAccess() {
    _access = false;
    if (_accessTrigger) _accessTrigger(_access);
}

}