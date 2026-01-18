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
#include <iostream>

// 3rd party
#include "cobs.h"
#include "crc.h"

// KC1FSZ stuff
#include "kc1fsz-tools/Common.h"

#include "DigitalAudioPortRxHandler.h"

using namespace std;

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
                _processEncodedMsg(_completeMsg, _completeMsgLen, cb);
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

    assert(encodedBufLen == NETWORK_MESSAGE_SIZE);

    // Decode the payload.
    uint8_t payload[PAYLOAD_SIZE];
    int rc = decodeMsg(encodedBuf, encodedBufLen, payload, PAYLOAD_SIZE);
    if (rc == 0) {
        cb(payload, PAYLOAD_SIZE);
        _rxCount++;
    } else {
        _badCount++;
    }
}

void DigitalAudioPortRxHandler::encodeCrc(int16_t crc, uint8_t* crc3) {
    crc3[0] = 0x01;
    pack_int16_le(crc, crc3 + 1);
    // Check for a zero bytes and fix them
    if (crc3[1] == 0) {
        crc3[0] |= 0x80;
        crc3[1] = 0x0ff;
    }
    if (crc3[2] == 0) {
        crc3[0] |= 0x40;
        crc3[2] = 0x0ff;
    }
}

int16_t DigitalAudioPortRxHandler::decodeCrc(const uint8_t* crc3) {
    uint8_t temp[2] = { crc3[1], crc3[2] };
    if (crc3[0] & 0x80)
        temp[0] = 0;
    if (crc3[0] & 0x40)
        temp[1] = 0;
    return unpack_int16_le(temp);
}

void DigitalAudioPortRxHandler::encodeMsg(
    const uint8_t* payload, unsigned payloadLen,
    uint8_t* msg, unsigned msgLen) {

    assert(payloadLen == PAYLOAD_SIZE);
    assert(msgLen == NETWORK_MESSAGE_SIZE);

    msg[0] = HEADER_CODE;
    // Flags
    msg[1] = 0x01;
    // Payload is encoded using COBS
    cobs_encode_result re = cobs_encode(msg + 3, NETWORK_MESSAGE_SIZE - 3,
        payload, PAYLOAD_SIZE);
    assert(re.status == COBS_ENCODE_OK);
    // This is the case where the COBS overhead is 1
    if (re.out_len == PAYLOAD_SIZE + 1) {
        // Flag the short COBS case
        msg[2] = 1;
        // Mark the overhead byte that wasn't used
        msg[3 + PAYLOAD_SIZE + 1] = 0xff;
    } else if (re.out_len == PAYLOAD_SIZE + 2) {
        // Flag the long COBS case
        msg[2] = 2;
    } else {
        assert(false);
    }
    // CRC is everything, including the header
    int16_t crc = crcSlow(msg, 1 + FLAGS_LEN + PAYLOAD_SIZE + COBS_OVERHEAD);
    encodeCrc(crc, msg + NETWORK_MESSAGE_SIZE - 3);
}

int DigitalAudioPortRxHandler::decodeMsg(
    const uint8_t* msg, unsigned msgLen,
    uint8_t* payload, unsigned payloadLen) {    

    assert(msgLen == NETWORK_MESSAGE_SIZE);
    assert(payloadLen == PAYLOAD_SIZE);

    // Header
    if (msg[0] != HEADER_CODE)
        return -1;
    // CRC is everything, including the header
    int16_t expectedCrc = crcSlow(msg, 1 + FLAGS_LEN + PAYLOAD_SIZE + COBS_OVERHEAD);
    int16_t crc = decodeCrc(msg + NETWORK_MESSAGE_SIZE - 3);
    if (crc != expectedCrc)
        return -2;
    if (msg[1] != 0x01)
        return -3;
    unsigned cobsOverhead = msg[2];
    if (!(cobsOverhead == 1 || cobsOverhead == 2))
        return -4;
    cobs_decode_result rd = cobs_decode(payload, PAYLOAD_SIZE,
        msg + 3, PAYLOAD_SIZE + cobsOverhead);
    if (rd.status != COBS_DECODE_OK)
        return -5;
    if (rd.out_len != PAYLOAD_SIZE)
        return -6;
    return 0;
}

}
