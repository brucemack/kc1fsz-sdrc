#pragma once

#include <cstdint>

namespace kc1fsz {

int processRxBuf(uint8_t* rxBuf, uint8_t** nextReadPtr,
    uint8_t* dmaWritePtr, unsigned bufSize,
    std::function<void(const uint8_t* buf, unsigned bufLen)> cb);

void networkAudioReceiveIfAvailable(
    std::function<void(const uint8_t* audioFrame, unsigned len)> cb);

void networkAudioSend(const uint8_t* audioFrame, unsigned len);

}

