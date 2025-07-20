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
#include "AudioCore.h"
#include "CourtesyToneGenerator.h"
#include "IDToneGenerator.h"
#include "TestToneGenerator.h"

namespace kc1fsz {

class AudioCore;

class TxControl {
public:

    TxControl(Clock& clock, Log& log, Tx& tx, AudioCore& core);

    virtual void run();

    /**
     * @brief Makes a receiver visible to the transmitter.
     */
    void setRx(unsigned i, Rx* rx);

    /**
     * @brief Controls whether a receiver is visible to the transmitter.
     */
    void setRxEligible(unsigned i, bool enabled);

    void forceId() { _enterPreId(); }
    void startTest() { _enterTest(); }
    int getState() const { return (int)_state; }
   
    void setCall(const char* callSign) { _idToneGenerator.setCall(callSign); }
    void setPass(const char* pass) {  }
    void setTimeoutTime(uint32_t ms) { _timeoutWindowMs = ms; }
    void setLockoutTime(uint32_t ms) { _lockoutWindowMs = ms; }
    void setHangTime(uint32_t ms) { _hangWindowMs = ms; }
    void setCtMode(CourtesyToneGenerator::Type mode) { _courtesyToneGenerator.setType(mode); }
    void setCtLevel(float db) { _courtesyToneGenerator.setLevel(db); }
    void setIdMode(int mode) { _idMode = mode; }
    void setIdLevel(float db) { _idToneGenerator.setLevel(db); } 
    void setIdRequiredInt(uint32_t sec) { _idRequiredIntSec = sec; }
    void setDiagToneFreq(float hz) { _testToneGenerator.setFreq(hz); }
    void setDiagToneLevel(float dbv) { _testToneGenerator.setLevel(dbv); }

private:

    enum State { INIT, IDLE, VOTING, ACTIVE, PRE_ID, ID, POST_ID, 
        ID_URGENT, PRE_COURTESY, COURTESY, HANG, LOCKOUT, TEST };

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
    void _enterActive();
    void _enterPreId();
    void _enterId();
    void _enterPostId();
    void _enterIdUrgent();
    void _enterPreCourtesy();
    void _enterCourtesy();
    void _enterHang();
    void _enterLockout();
    void _enterTest();

    Clock& _clock;
    Log& _log;
    Tx& _tx;
    AudioCore& _audioCore;

    State _state = State::INIT;   

    const static unsigned int MAX_RX_COUNT = 8;

    // This is where the bindings to the various receivers is stored
    Rx* _rx[MAX_RX_COUNT] = { 
        0, 0, 0, 0, 
        0, 0, 0, 0 };
    // These flags indicate which receivers are eligible to be 
    // repeated through this transmitter. This is controlled by the 
    // user configuration.
    bool _rxEligible[MAX_RX_COUNT] = { 
        false, false, false, false, 
        false, false, false, false };
    // These flags indicate which receivers have been selected for repeating.
    // It is set based on the "voting" logic that is determined when leaving
    // idle state.
    bool _rxSelected[MAX_RX_COUNT] = { 
        false, false, false, false, 
        false, false, false, false };

    /**
     * @brief Checks to see if there is any receive activity amongst the 
     * receivers that are marked as eligible.
    */
    bool _anyRxActivityAmongstEligible() const;
    void _clearSelected();

    CourtesyToneGenerator _courtesyToneGenerator;
    IDToneGenerator _idToneGenerator;
    TestToneGenerator _testToneGenerator;

    uint32_t _lastIdleStartTime = 0;
    uint32_t _timeoutTime = 0;
    uint32_t _lastCommunicationTime = 0;
    uint32_t _lastIdTime = 0;
    uint32_t _lastActiveReceiver = 0;

    uint32_t _currentStateEndTime = 0;

    // ----- Configurations 

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
    uint32_t _idRequiredIntSec = 60 * 10; 
    // Length of the grace period before we raise an urgent ID
    uint32_t _idGraceWindowMs = 1000 * 15;
    // ID mode
    int _idMode = 1;
};

}

#endif
