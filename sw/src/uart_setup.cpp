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
#include <cstdio>
#include <cstring>
#include <cmath>
/*
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
*/
#include <functional>
#include <iostream>

#include "kc1fsz-tools/CobsCodec.h"
#include "i2s_setup.h"

using namespace std;

namespace kc1fsz {

// 1 header null + 128 message + 1 COBS overhead + 4 control
static const unsigned FIXED_MESSAGE_SIZE = 134;

static unsigned avail(unsigned readIx, unsigned writeIx,
    unsigned bufSize) {
    if (writeIx >= readIx)
        return writeIx - readIx;
    else
        // Move the read pointer forward by one full buffer
        // to address the wrapping problem.
        return (readIx + bufSize) - writeIx;
}

int processRxBuf(uint8_t* rxBuf, uint8_t** nextReadPtr,
    uint8_t* dmaWritePtr, unsigned bufSize,
    std::function<void(const uint8_t* buf, unsigned bufLen)> cb) {
    // Scan up to a header byte
    while (*nextReadPtr != dmaWritePtr && **nextReadPtr != 0) {
        (*nextReadPtr)++;
        // Look for wrap
        if ((*nextReadPtr) == (rxBuf + bufSize))
            *nextReadPtr = rxBuf;
    }
    // If no header byte is encountered then come back next time
    if (**nextReadPtr != 0)
        return 1;
    // Check to see if we have enough for a full message
    unsigned a = avail(*nextReadPtr - rxBuf, 
        dmaWritePtr - rxBuf, bufSize);
    if (a < FIXED_MESSAGE_SIZE)
        return 2;
    // Decode the message. Don't need the header or the overhead
    uint8_t decodeBuf[FIXED_MESSAGE_SIZE - 2];
    int rc2 = cobsDecode(*nextReadPtr + 1, FIXED_MESSAGE_SIZE - 1,
        decodeBuf, sizeof(decodeBuf));
    // Fire the callback to transfer the decoded message
    if (rc2 == 0)
        cb(decodeBuf, sizeof(decodeBuf));
    // Advance the read pointer and deal with wrap
    *nextReadPtr += FIXED_MESSAGE_SIZE;
    if (*nextReadPtr >= (rxBuf + bufSize))
        *nextReadPtr -= bufSize;
    return 0;
}

void networkAudioReceiveIfAvailable(
    std::function<void(const uint8_t* buf, unsigned bufLen)> cb) {
}

void networkAudioSend(const uint8_t* audio8KLE, unsigned len) { 
}

}
