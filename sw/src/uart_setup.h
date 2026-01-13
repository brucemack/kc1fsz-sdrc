#pragma once

#include <cstdint>

namespace kc1fsz {

void streaming_uart_setup();

int processRxBuf(uint8_t* rxBuf, uint8_t** nextReadPtr,
    const uint8_t* dmaWritePtr, unsigned bufSize,
    std::function<void(const uint8_t* buf, unsigned bufLen)> cb);

void networkAudioReceiveIfAvailable(
    std::function<void(const uint8_t* audioFrame, unsigned len)> cb);

/**
 * @param audioFrame Does not include header. 
 * @param len At this point (64 * 2) + 4
 */
void networkAudioSend(const uint8_t* audioFrame, unsigned len);

}

