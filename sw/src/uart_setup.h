#pragma once

#include <cstdint>

namespace kc1fsz {

int processRxBuf(uint8_t* rxBuf, uint8_t** nextReadPtr,
    uint8_t* dmaWritePtr, unsigned bufSize,
    std::function<void(const uint8_t* buf, unsigned bufLen)> cb);

}

