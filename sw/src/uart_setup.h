#pragma once

#include <cstdint>

#define COBS_OVERHEAD (2)
#define CRC_LEN (2)
#define PAYLOAD_SIZE (160 * 2)
// Fixed size, 1 header null + audio + CRC + COBS overhead
#define NETWORK_MESSAGE_SIZE (1 + PAYLOAD_SIZE + CRC_LEN + COBS_OVERHEAD)
#define HEADER_CODE (0)

namespace kc1fsz {

void streaming_uart_setup();

typedef void (*receive_processor)(const uint8_t* buf, unsigned len);

/**
 * Called on every audio tick. Checks for inbound network audio and 
 * dispatches the callback if any is available.
 */
void networkAudioReceiveIfAvailable(receive_processor cb);

/**
 * @param audioFrame Does not include header. 
 * @param len At this point (160 * 2)
 */
void networkAudioSend(const uint8_t* audioFrame, unsigned len);

class AudioPortRxHandler {
public:

    AudioPortRxHandler(uint8_t* rxBuf, unsigned rxBufSize);

    /**
     * Processes data in the circular buffer and fires the callback for 
     * each successfully received/decoded message.
     */
    void processRxBuf(unsigned nextWrPtr,
        std::function<void(const uint8_t* msg, unsigned msgLen)> cb);

    /**
     * @returns The number of clean messages received.
     */
    unsigned getRxCount() const { return _rxCount; }

private:

    void _processEncodedMsg(
        const uint8_t* encodedBuf, unsigned encodedBufLen,
        std::function<void(const uint8_t* msg, unsigned msgLen)> cb);

    uint8_t* const _rxBuf;
    const unsigned _rxBufSize;
    unsigned _rxBufMask;
    unsigned _nextRdPtr;
    bool _haveHeader = false;
    uint8_t _completeMsg[NETWORK_MESSAGE_SIZE];
    unsigned _completeMsgLen = 0;
    unsigned _rxCount = 0;
};

}

