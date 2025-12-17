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
#include "AudioCoreOutputPort.h"

namespace kc1fsz {

TxControl::TxControl(Clock& clock, Log& log, Tx& tx, AudioCoreOutputPort& core)
:   _clock(clock),
    _log(log),
    _tx(tx),
    _courtesyToneGenerator(log, clock, core),
    _idToneGenerator(log, clock, core),
    _testToneGenerator(log, clock, core),
    _audioCore(core)
{
}

void TxControl::run() { 

    // Advance sub-components
    _idToneGenerator.run();
    _courtesyToneGenerator.run();
    _testToneGenerator.run();

    // ----- Transmitter State Machine ----------------------------------------

    if (_state == State::INIT) {
        _enterIdle();
    }
    else if (_state == State::IDLE) {
        if (_isIdRequired(false)) {
            _enterPreId();
        }
        // Check all of the receivers for activity. If anything happens then enter
        // the active mode.
        else if (_audioCore.isAudioActive()) {
            //_log.info("RX activity seen");
            _enterActive();
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
    else if (_state == State::ACTIVE) {

        // Keep on updating the timestamp
        _lastCommunicationTime = _clock.time();     

        // Look for timeout
        if (_timeoutTime != 0 && _clock.isPast(_timeoutTime)) {
            _log.info("Timeout detected, lockout start");
            _enterLockout();
        } 

        // TODO: Look for urgent ID situation
        //else if (_isIdRequired(true)) {            
        //}

        // Look for unkey of all active receivers.
        else if (!_audioCore.isAudioActive()) {
            _log.info("Receiver COS dropped");
            _log.info("Pause before courtesy");
            // The type of courtesy tone is a function of the last receiver
            _courtesyToneGenerator.setType(_tx.getCourtesyType());
            _enterPreCourtesy();
        } 

        // NOTICE: No matter how we leave the ACTIVE state, the selection
        // has been cleared.
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
        else if (_audioCore.isAudioActive()) {
            _log.info("RX activity, cancelled courtesy");
            _enterActive();
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
        // and will jump back into the active state.
        else if (_audioCore.isAudioActive()) {
            _log.info("RX activity, hang cancelled");
            _enterActive();
        }
    }
    // In this state we are waiting a defined period of 
    // time, during which nothing can happen. 
    else if (_state == State::LOCKOUT) {
        // There is nothing that can happen until the timeout passes.
        // Look for end of lockout time
        if (_isStateTimedOut()) {
            // Per Dan W1DAN's suggestion, before leaving lock-out, recheck 
            // to see if there is activty (ex: stuck transmitter). If so
            // extend again.
            if (_audioCore.isAudioActive()) {
                _log.info("Lockout extended");
                // Should be needed, but just in case
                _tx.setPtt(false);
                // Doing this just to extend the timeout
                _setState(State::LOCKOUT, _lockoutWindowMs);
            }
            else {
                _log.info("Lockout end");
                _enterPreId();
            }
        }
    }
    else if (_state == State::TEST) {
        // Look for timeout
        if (_clock.isPast(_timeoutTime)) {
            _log.info("Timeout detected, lockout start");
            _enterLockout();
        } 
        else if (_testToneGenerator.isFinished()) {
            _enterIdle();
        }
    }
}

void TxControl::setMute(bool mute) {
    if (mute) {
        _enterIdle();
        // Go park in a state where nothing will happen
        _state = State::TEMPORARY_MUTE;
    } else {
        _enterIdle();
    }
}

void TxControl::_enterIdle() {
    _state = State::IDLE;
    _lastIdleStartTime = _clock.time();
    _tx.setPtt(false);
    _testToneGenerator.stop();
}

void TxControl::_enterTest() {
    _tx.setPtt(true);
    _testToneGenerator.start();
    if (_timeoutWindowMs)
        _timeoutTime = _clock.time() + _timeoutWindowMs;
    else
        _timeoutTime = 0;
    _setState(State::TEST, 0);
}

void TxControl::_enterActive() {
    _setState(State::ACTIVE, 0);
    if (_timeoutWindowMs)
        _timeoutTime = _clock.time() + _timeoutWindowMs;
    else
        _timeoutTime = 0;
    // Reset the audio delay since we are about to start passing 
    // audio through the system.
    _audioCore.resetDelay();
    // Key the transmitter
    _tx.setPtt(true);
}

void TxControl::_enterPreId() {
    _tx.setPtt(true);
    _setState(State::PRE_ID, _preIdWindowMs);
}

void TxControl::_enterId() {
    _lastIdTime = _clock.time();
    _tx.setPtt(true);
    _idToneGenerator.start();
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
    // 0. ID mode must be enabled
    // 1. Any communication has happened since the last ID
    // 2  a. We just booted up OR
    //    b. It's been more than 10 minutes since the last ID
    // 3. The pause window has passed to make sure that we don't step 
    //    on an active QSO.
    if (_idMode == 1 &&
        _lastCommunicationTime > _lastIdTime && 
        (_lastIdTime == 0 || _clock.isPast(_lastIdTime + (_idRequiredIntSec * 1000))) &&
        _clock.isPast(_lastIdleStartTime + _quietWindowMs)) {
        return true;
    } else {
        return false;
    }
}

}
