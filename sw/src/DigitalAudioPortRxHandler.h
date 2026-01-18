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
#pragma once

#include <cstdint>
#include <functional>

#define HEADER_CODE (0)
#define PAYLOAD_SIZE (160 * 2)
#define FLAGS_LEN (2)
#define COBS_OVERHEAD (2)
// 16-bit CRC with extra to avoid zeros
#define CRC_LEN (3)
#define NETWORK_MESSAGE_SIZE (1 + FLAGS_LEN + PAYLOAD_SIZE + COBS_OVERHEAD + CRC_LEN)

namespace kc1fsz {

/**
 * Contains the read pointer for the circular receive buffer. The
 * write pointer is external because it is typically being maintained
 * by a DMA controller or something similar.
 */
class DigitalAudioPortRxHandler {
public:

    /**
     * @param rxBuf Receive buffer, typically shared with a DMA controller.
     * @param rxBufSize Size of the circular receive buffer. Must be a 
     * power of two!!
     */
    DigitalAudioPortRxHandler(uint8_t* rxBuf, unsigned rxBufSize);

    /**
     * Processes data in the circular buffer and fires the callback for 
     * each successfully received/decoded message.
     *
     * @param nextWrPtr The external write pointer, which is typically
     * taken from a DMA controller or something.
     */
    void processRxBuf(unsigned nextWrPtr,
        std::function<void(const uint8_t* msg, unsigned msgLen)> cb);

    /**
     * @returns The number of clean messages received.
     */
    unsigned getRxCount() const { return _rxCount; }

    /**
     * @returns The number of bad messages discarded due to 
     * CRC errors or some other malformation.
     */
    unsigned getBadCount() const { return _badCount; }

    /**
     * Turns a 16-bit CRC value into 3-bytes with no zeros.
     */
    static void encodeCrc(int16_t crc, uint8_t* crc3);

    /**
     * Turns 3-bytes into a 16-bit CRC value. 
     */
    static int16_t decodeCrc(const uint8_t* crc3);

    /**
     * Encodes a payload into a complete message, including the 
     * leading header byte.
     */
    static void encodeMsg(const uint8_t* payload, unsigned playloadLen,
        uint8_t* msg, unsigned msgLen);

    /**
     * Decodes a message starting with (and including) the leading
     * header byte.
     * @returns -1 If the message is invalid
     */
    static int decodeMsg(const uint8_t* msg, unsigned msgLen,
        uint8_t* payload, unsigned payloadLen);

private:

    void _processEncodedMsg(
        const uint8_t* encodedBuf, unsigned encodedBufLen,
        std::function<void(const uint8_t* msg, unsigned msgLen)> cb);

    uint8_t* const _rxBuf;
    const unsigned _rxBufSize;
    unsigned _nextRdPtr;
    const unsigned _rxBufMask;
    bool _haveHeader = false;
    uint8_t _completeMsg[NETWORK_MESSAGE_SIZE];
    unsigned _completeMsgLen = 0;
    unsigned _rxCount = 0;
    unsigned _badCount = 0;
};

}
