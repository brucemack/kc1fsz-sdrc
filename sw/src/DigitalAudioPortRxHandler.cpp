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
 */
#include <cstring>
#include <cassert>

// 3rd party
#include "cobs.h"
#include "crc.h"

// KC1FSZ stuff
#include "kc1fsz-tools/Common.h"

#include "DigitalAudioPortRxHandler.h"

namespace kc1fsz {

DigitalAudioPortRxHandler::DigitalAudioPortRxHandler(uint8_t* rxBuf, unsigned rxBufSize)
:   _rxBuf(rxBuf),
    _rxBufSize(rxBufSize),
    _nextRdPtr(0), 
    _rxBufMask(sizeToBitMask(rxBufSize)) {
}

void DigitalAudioPortRxHandler::processRxBuf(unsigned nextWrPtr,
    std::function<void(const uint8_t* msg, unsigned msgLen)> cb) {

    bool firedCb = false;

    while (!firedCb && _nextRdPtr != nextWrPtr) {
        assert(_nextRdPtr < _rxBufSize);
        if (_rxBuf[_nextRdPtr] == HEADER_CODE) {
            // When a header is found we reset the accumulation and 
            // start again.
            _completeMsg[0] = 0;
            _completeMsgLen = 1;
            _haveHeader = true;
        } else if (_haveHeader) {
            // Keep accumulating a potential message
            _completeMsg[_completeMsgLen++] = _rxBuf[_nextRdPtr];
            // Did we get a full message?
            if (_completeMsgLen == NETWORK_MESSAGE_SIZE) {
                // Skipping the header byte
                _processEncodedMsg(_completeMsg + 1, _completeMsgLen - 1, cb);
                firedCb = true;
                _haveHeader = false;
            }
        } 
        _nextRdPtr = (_nextRdPtr + 1) & _rxBufMask;
    }
}

void DigitalAudioPortRxHandler::_processEncodedMsg(
    const uint8_t* encodedBuf, unsigned encodedBufLen,
    std::function<void(const uint8_t* msg, unsigned msgLen)> cb) {

    assert(encodedBufLen == NETWORK_MESSAGE_SIZE - 1);

    // Decode the COBS message. Don't need space for the COBS overhead
    // that is being stripped off.
    uint8_t decodedBuf[PAYLOAD_SIZE + CRC_LEN];
    cobs_decode_result rd = cobs_decode(decodedBuf, sizeof(decodedBuf), 
        encodedBuf, encodedBufLen);
    if (rd.status != COBS_DECODE_OK ||
        rd.out_len != sizeof(decodedBuf)) {
        _badCount++;
        return;
    }

    // CRC check
    int16_t crcExpected = crcSlow(decodedBuf, PAYLOAD_SIZE);
    int16_t crcActual = unpack_int16_le(decodedBuf + PAYLOAD_SIZE);
    if (crcActual != crcExpected) {
        _badCount++;
        return;
    }

    cb(decodedBuf, PAYLOAD_SIZE);
    _rxCount++;
}


}
