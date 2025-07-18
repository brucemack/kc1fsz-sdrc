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
#include <cassert>

#include "TxControl.h"
#include "AudioCore.h"

namespace kc1fsz {

TxControl::TxControl(Clock& clock, Log& log, Tx& tx, AudioCore& core)
:   _clock(clock),
    _log(log),
    _tx(tx),
    _courtesyToneGenerator(log, clock, core),
    _idToneGenerator(log, clock, core),
    _audioCore(core)
{
}

void TxControl::setRx(unsigned i, Rx* rx) {
    assert(i < MAX_RX_COUNT);
    _rx[i] = rx;
}

void TxControl::setRxEligible(unsigned i, bool b) {
    assert(i < MAX_RX_COUNT);
    _rxEligible[i] = b;
}

void TxControl::_clearActive() {
    for (unsigned i = 0; i < MAX_RX_COUNT; i++)
        _rxActive[i] = false;
}

unsigned TxControl::_adjustGains() {

    // Keep track of the last active receiver
    unsigned activeCount = 0;
    bool active[MAX_RX_COUNT];
    for (unsigned i = 0; i < MAX_RX_COUNT; i++) {
        if (_rx[i] != 0 && _rxEligible[i] && _rxActive[i] && _rx[i]->isActive()) {
            _lastActiveReceiver = i;
            active[i] = true;
            activeCount++;
        }
        else {
            active[i] = false;
        }
    }

    // Divide the gain evenly across the active receivers
    float gain = 1.0 / (float)activeCount;
    for (unsigned i = 0; i < MAX_RX_COUNT; i++) {
        if (active[i]) 
            _rx[i]->setGainLinear1(gain);
        else 
            _rx[i]->setGainLinear1(0);
    }
   
    return activeCount;
}

void TxControl::run() { 

    // Advance sub-components
    _idToneGenerator.run();
    _courtesyToneGenerator.run();

    if (_state == State::INIT) {
        _enterIdle();
    }
    else if (_state == State::IDLE) {
        if (_isIdRequired(false)) {
            _enterPreId();
        }
        // Check all of the receivers for activity. If anything happens then enter
        // the voting mode to decide which receiver to focus on.
        else if (_anyRxActivityAmongstEligible()) {
            _log.info("RX activity seen");
            _enterVoting();
        }
    }
    // In this state we are pausing before sending the CW ID.  Nothing
    // can interrupt.
    else if (_state == State::PRE_ID) {
        if (_isStateTimedOut()) {
            _enterId();
        }
    }
    // In this state we are sending the CW ID.  Nothing
    // can interrupt.
    else if (_state == State::ID) {
        // Is the tone finished sending?
        if (_idToneGenerator.isFinished()) {
            _enterPostId();
        }
    }
    // In this state we are pausing after sending the CW ID.  Nothing
    // can interrupt.
    else if (_state == State::POST_ID) {
        if (_isStateTimedOut()) {
            _enterIdle();
        }
    }
    // In this state we collect receiver status and decide
    // which receiver to select. 
    else if (_state == State::VOTING) {
        // Make decision on timeout
        if (_isStateTimedOut()) {
            // TODO: Current implementation is first-come-first-served
            bool anyActive = false;
            for (unsigned int i = 0; i < MAX_RX_COUNT; i++) {
                if (_rx[i] != 0 && _rxEligible[i] && _rx[i]->isActive()) {
                    _log.info("Receiver is selected [%d]", _rx[i]->getId());
                    _rxActive[i] = true;
                    _lastActiveReceiver = i;
                    _enterActive();
                    anyActive = true;
                    break;
                }
            }
            if (!anyActive) 
                _enterIdle();
        }
    }
    else if (_state == State::ACTIVE) {

        // Keep on updating the timestamp
        _lastCommunicationTime = _clock.time();     

        unsigned activeCount = _adjustGains();

        // Look for timeout
        if (_clock.isPast(_timeoutTime)) {
            _log.info("Timeout detected, lockout start");
            _clearActive();
            _adjustGains();
            _enterLockout();
        } 
        // Look for urgent ID situation
        //else if (_isIdRequired(true)) {            
        //}
        // Look for unkey of all active receivers.
        else if (activeCount == 0) {
            _log.info("Receiver COS dropped [%u]", _lastActiveReceiver);
            _log.info("Pause before courtesy");
            if (_rx[_lastActiveReceiver] != 0)
                _courtesyToneGenerator.setType(_rx[_lastActiveReceiver]->getCourtesyType());
            _clearActive();
            _adjustGains();
            _enterPreCourtesy();
        } 
    }
    // In this state we wait a bit to make sure nobody
    // else is talking and then trigger the courtesy tone.
    else if (_state == State::PRE_COURTESY) {
        if (_isStateTimedOut())  {
            _log.info("Courtesy tone start");
            _enterCourtesy();
        }
        // Check to see if the previously active receiver
        // has come back (i.e. debounce)
        else if (_anyRxActivityAmongstEligible()) {
            _log.info("RX activity, cancelled courtesy");
            _enterVoting();
        }
    }
    // In this state we are waiting for the courtesy
    // tone to complete. Nothing can interrupt this.
    else if (_state == State::COURTESY) {
        if (_courtesyToneGenerator.isFinished()) {
            _log.info("Courtesy tone end, hang start");
            _enterHang();
        }
    }
    // In this state we are waiting a bit before allowing
    // the transmitter to drop. This can be interrupted 
    // by another station transmitting.
    else if (_state == State::HANG) {
        if (_isStateTimedOut())  {
            _log.info("Hang ended");
            _enterIdle();
        }
        // Any receive activity will end the hang period
        // and will jump back into the voting state.
        else if (_anyRxActivityAmongstEligible()) {
            _log.info("RX activity, hang cancelled");
            _enterVoting();
        }
    }
    // In this state we are waiting a defined period of 
    // time, during which nothing can happen. 
    else if (_state == State::LOCKOUT) {
        // Any time we have activity the lockout phase gets restarted
        if (_anyRxActivityAmongstEligible()) {
            _enterLockout();
        }
        // Look for end of lockout time
        else if (_isStateTimedOut()) {
            _log.info("Lockout end");
            _enterPreId();
        }
    }
}

bool TxControl::_anyRxActivityAmongstEligible() const {
    for (unsigned i = 0; i < MAX_RX_COUNT; i++)
        if (_rx[i] != 0 && _rxEligible[i] && _rx[i]->isActive())
            return true;
    return false;
}

void TxControl::_enterIdle() {
    _state = State::IDLE;
    _lastIdleStartTime = _clock.time();
    _tx.setPtt(false);
    // Clean slate
    for (unsigned i = 0; i < MAX_RX_COUNT; i++) {
        _rxActive[i] = false;
        if (_rx[i] != 0)
            _rx[i]->setGainLinear1(0);
    }
}

void TxControl::_enterVoting() {
    _setState(State::VOTING, _votingWindowMs);
    // Clean slate
    for (unsigned i = 0; i < MAX_RX_COUNT; i++)
        _rxActive[i] = false;
}

void TxControl::_enterActive() {
    _setState(State::ACTIVE, 0);
    _timeoutTime = _clock.time() + _timeoutWindowMs;
    // Key the transmitter
    _tx.setPtt(true);
}

void TxControl::_enterPreId() {
    _tx.setPtt(true);
    _setState(State::PRE_ID, _preIdWindowMs);
}

void TxControl::_enterId() {
    _tx.setPtt(true);
    _idToneGenerator.start();
    _lastIdTime = _clock.time();
    _setState(State::ID, 0);
}

void TxControl::_enterPostId() {
    _tx.setPtt(true);
    _setState(State::POST_ID, _postIdWindowMs);
}

void TxControl::_enterIdUrgent() {
    _tx.setPtt(true);
    _idToneGenerator.start();
    _lastIdTime = _clock.time();
    _setState(State::ID_URGENT, 0);
}

void TxControl::_enterPreCourtesy() {
    _setState(PRE_COURTESY, _preCourtseyWindowMs);
}

void TxControl::_enterCourtesy() {
    _courtesyToneGenerator.start();
    _setState(State::COURTESY, 0);
}

void TxControl::_enterHang() {
    _setState(State::HANG, _hangWindowMs);
}

void TxControl::_enterLockout() {
    _tx.setPtt(false);
    _setState(State::LOCKOUT, _lockoutWindowMs);
}

void TxControl::_setState(State state, uint32_t timeoutWindowMs) {
    _state = state;
    if (timeoutWindowMs != 0)
        _currentStateEndTime = _clock.time() + timeoutWindowMs;
    else
        _currentStateEndTime = 0;
}

bool TxControl::_isStateTimedOut() const { 
    return _currentStateEndTime != 0 && _clock.isPast(_currentStateEndTime); 
}

bool TxControl::_isIdRequired(bool inQso) const {
    // Check to see if it's time to send the ID from idle. We ID
    // if all of these conditions are met:
    // 1. Any communication has happened since the last ID
    // 2  a. We just booted up OR
    //    b. It's been more than 10 minutes since the last ID
    // 3. The pause window has passed to make sure that we don't step 
    //    on an active QSO.
    if (_lastCommunicationTime > _lastIdTime && 
        (_lastIdTime == 0 || _clock.isPast(_lastIdTime + (_idRequiredIntSec * 1000))) &&
        _clock.isPast(_lastIdleStartTime + _quietWindowMs)) {
        return true;
    } else {
        return false;
    }
}

}
