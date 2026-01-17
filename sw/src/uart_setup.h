#pragma once

#include <cstdint>

namespace kc1fsz {

void streaming_uart_setup();

typedef void (*receive_processor)(const uint8_t* buf, unsigned len);

/**
 * Called on every audio tick. Checks for inbound network audio and 
 * dispatches the callback if any is available.
 */
void networkAudioReceiveIfAvailable(receive_processor cb);

/**
 * @param audioFrame Does not include header/CRC/COBS
 * @param len At this point (160 * 2)
 */
void networkAudioSend(const uint8_t* audioFrame, unsigned len);

}

