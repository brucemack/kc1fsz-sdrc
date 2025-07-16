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
#ifndef _TxControl_h
#define _TxControl_h

#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/Clock.h"

#include "Tx.h"
#include "Rx.h"
#include "CourtesyToneGenerator.h"
#include "IDToneGenerator.h"
#include "VoiceGenerator.h"
#include "AudioSourceControl.h"

namespace kc1fsz {

class AudioCore;

class TxControl {
public:

    TxControl(Clock& clock, Log& log, Tx& tx, AudioCore& core,
        AudioSourceControl& audioSource);

    virtual void run();

    /**
     * Makes a receiver visible to the transmitter.
     */
    void setRx(unsigned int i, Rx* rx);

    void forceId() {
        _enterPreId();
    }

    enum RepeatMode { MODE_INDEPENDENT, MODE_EXCLUSIVE, MODE_MIX };
    enum CtMode { CT_NONE, CT_SINGLE, CT_UPCHIRP, CT_DOWNCHRIP };

    void setCall(const char* callSign) { _idToneGenerator.setCall(callSign); }
    void setPass(const char* pass) {  }
    void setRepeatMode(RepeatMode mode) { _repeatMode = mode; }
    void setTimeoutTime(uint32_t ms) { _timeoutWindowMs = ms; }
    void setLockoutTime(uint32_t ms) { _lockoutWindowMs = ms; }
    void setHangTime(uint32_t ms) { _hangWindowMs = ms; }
    void setCtMode(CtMode mode) { _ctMode = mode; }
    void setCtLevel(float db) { _courtesyToneGenerator.setLevel(db); }
    void setIdLevel(float db) { _idToneGenerator.setLevel(db); } 

private:

    enum State { INIT, IDLE, VOTING, ACTIVE, PRE_ID, ID, POST_ID, ID_URGENT, PRE_COURTESY, COURTESY, HANG, LOCKOUT };

    void _setState(State state, uint32_t timeoutWindowMs = 0);
    bool _isStateTimedOut() const;

    /**
     * @returns An indication of whether an ID needs to be sent now. This
     * is slightly dependent on whether a QSO is active since we can have
     * a grace period.
     */
    bool _isIdRequired(bool qsoActive) const;

    void _enterIdle();
    void _enterVoting();
    void _enterActive(Rx* rx);
    void _enterPreId();
    void _enterId();
    void _enterPostId();
    void _enterIdUrgent();
    void _enterPreCourtesy();
    void _enterCourtesy();
    void _enterHang();
    void _enterLockout();
    bool _anyRxActivity() const;

    Clock& _clock;
    Log& _log;
    Tx& _tx;
    AudioSourceControl& _audioSource;

    State _state = State::INIT;   

    const static unsigned int _maxRxCount = 4;
    Rx* _rx[_maxRxCount] = { 0, 0, 0, 0 };
    Rx* _activeRx;

    CourtesyToneGenerator _courtesyToneGenerator;
    IDToneGenerator _idToneGenerator;
    //VoiceGenerator _idToneGenerator;

    uint32_t _lastIdleTime = 0;
    uint32_t _timeoutTime = 0;
    uint32_t _lastCommunicationTime = 0;
    uint32_t _lastIdTime = 0;

    uint32_t _currentStateEndTime = 0;
    //void (TxControl::*_currentStateTimeoutFuncPtr)() = 0;

    // ----- Configurations 

    RepeatMode _repeatMode = RepeatMode::MODE_INDEPENDENT;
    CtMode _ctMode = CtMode::CT_UPCHIRP;
    // Disabled for now
    uint32_t _votingWindowMs = 25;
    // How long between the end of transmission and the courtesy tone
    uint32_t _preCourtseyWindowMs = 1500;    
    // How long we pause with the transmitter keyed before sending the CWID
    uint32_t _preIdWindowMs = 1000;    
    // How long we pause with the transmitter keyed after sending the CWID
    uint32_t _postIdWindowMs = 1000;    
    // How long a transmitter is allowed to stay active
    uint32_t _timeoutWindowMs = 1000 * 120;    
    // How long we sleep after a timeout is detected
    uint32_t _lockoutWindowMs = 1000 * 60;
    // Length of hang interval
    uint32_t _hangWindowMs = 1000 * 2;
    // Amount of time that passes in the idle state before we decide the 
    // repeater has gone quiet
    uint32_t _quietWindowMs = 1000 * 5;
    // Time between mandatory IDs
    uint32_t _idRequiredWindowMs = 1000 * 60 * 10; 
    // Length of the grace period before we raise an urgent ID
    uint32_t _idGraceWindowMs = 1000 * 15;
};

}

#endif
