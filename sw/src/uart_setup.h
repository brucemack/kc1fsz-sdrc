#pragma once

#include <cstdint>

namespace kc1fsz {

void streaming_uart_setup();

typedef void (*receive_processor)(const uint8_t* buf, unsigned len);

void networkAudioReceiveIfAvailable(receive_processor cb);

/**
 * @param audioFrame Does not include header. 
 * @param len At this point (160 * 2)
 */
void networkAudioSend(const uint8_t* audioFrame, unsigned len);

}

