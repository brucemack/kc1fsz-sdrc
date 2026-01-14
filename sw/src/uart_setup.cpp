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

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

#include <functional>
#include <iostream>

#include "kc1fsz-tools/CobsCodec.h"
#include "uart_setup.h"

using namespace std;

namespace kc1fsz {

#define UART_RX_BUF_SIZE 512
// This buffer is used with the DMA ring feature so it needs to be power-of-two
// aligned.
static uint8_t __attribute__((aligned(UART_RX_BUF_SIZE))) rx_buf[UART_RX_BUF_SIZE];
#define UART_TX_BUF_SIZE 256
static uint8_t tx_buf[UART_TX_BUF_SIZE];

static uint dma_ch_tx = 0;
static uint dma_ch_rx = 0;
static bool enabled = false;

// This pointer keeps track of read progress
static uint8_t* nextReadPtr = rx_buf;

// 1 header null + 128 message + 1 COBS overhead + 4 control
static const unsigned FIXED_MESSAGE_SIZE = 134;

static void __not_in_flash_func(dma_tx_handler)() {
}

// IMPORTANT NOTE: This is running in an interrupt so move quickly!!!
static void __not_in_flash_func(dma_irq1_handler)() {   
    if (dma_channel_get_irq1_status(dma_ch_tx)) {
        dma_channel_acknowledge_irq1(dma_ch_tx);
        dma_tx_handler();
    }
}

void streaming_uart_setup() {

    uart_init(uart0, 1152000);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);

    dma_ch_rx = dma_claim_unused_channel(true);
    dma_channel_config c_rx = dma_channel_get_default_config(dma_ch_rx);
    // Single byte transfer
    channel_config_set_transfer_data_size(&c_rx, DMA_SIZE_8);
    // Always reading from static UART register
    channel_config_set_read_increment(&c_rx, false); 
    // Writing to increasing memory address    
    channel_config_set_write_increment(&c_rx, true); 
    // Tell the DMA what to be triggered by 
    channel_config_set_dreq(&c_rx, DREQ_UART0_RX);
    // 512 byte circular buffer. The "true" means that this applies
    // to the write address, which is what is relevant for receiving
    // data from the UART.
    channel_config_set_ring(&c_rx, true, 9);
    channel_config_set_enable(&c_rx, true);

    dma_channel_configure(
        dma_ch_rx,
        &c_rx,
        // Destination address        
        rx_buf,
        // Source address (UART Data Register)
        &uart_get_hw(uart0)->dr, 
        // Full size of receive buffer. Using RP2350 self-trigger to keep 
        // the transfer running continuously.
        dma_encode_transfer_count_with_self_trigger(UART_RX_BUF_SIZE),
        // Not enabled yet
        false);
    
    dma_ch_tx = dma_claim_unused_channel(true);
    dma_channel_config c_tx = dma_channel_get_default_config(dma_ch_tx);
    channel_config_set_transfer_data_size(&c_tx, DMA_SIZE_8);
    // Always write to static UART register
    channel_config_set_write_increment(&c_tx, false); 
    // Reading from increasing memory address    
    channel_config_set_read_increment(&c_tx, true); 
    // Tell the DMA what to be triggered by 
    channel_config_set_dreq(&c_tx, DREQ_UART0_TX);
    channel_config_set_enable(&c_tx, true);
    
    dma_channel_configure(
        dma_ch_tx,
        &c_tx,
        &uart_get_hw(uart0)->dr,
        tx_buf,
        UART_TX_BUF_SIZE,
        // Not started yet
        false);
    // Fire an interrupt when the transfer out is complete
    //dma_channel_set_irq1_enabled(dma_ch_tx, true);

    // Bind to the interrupt handler 
    //irq_set_exclusive_handler(DMA_IRQ_1, dma_irq1_handler);
    //irq_set_enabled(DMA_IRQ_1, true);
    
    // Start the ball rolling on the RX side
    dma_channel_start(dma_ch_rx);

    enabled = true;
}

/**
 * Calculates the number of bytes available for processing in a circular buffer.
 * This takes into account the wrap
 */
static unsigned avail(unsigned readIx, unsigned writeIx, unsigned bufSize) {
    if (writeIx >= readIx)
        return writeIx - readIx;
    else
        // Move the read pointer forward by one full buffer
        // to address the wrapping problem.
        return (readIx + bufSize) - writeIx;
}

int processRxBuf(uint8_t* rxBuf, uint8_t** nextReadPtr,
    const uint8_t* dmaWritePtr, unsigned bufSize,
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
    unsigned a = avail(*nextReadPtr - rxBuf, dmaWritePtr - rxBuf, bufSize);
    if (a < FIXED_MESSAGE_SIZE)
        return 2;
    // Move the complete message into a contiguous buffer
    uint8_t completeMsg[FIXED_MESSAGE_SIZE];
    for (unsigned i = 0; i < FIXED_MESSAGE_SIZE; i++) {
        completeMsg[i] = **nextReadPtr;
        // Advance and look for wrap
        (*nextReadPtr)++;
        if ((*nextReadPtr) == (rxBuf + bufSize))
            *nextReadPtr = rxBuf;
    }
    // Fire the callback and give the encoded message, minus the null header byte
    cb(completeMsg + 1, FIXED_MESSAGE_SIZE - 1);
    return 0;
}

void networkAudioReceiveIfAvailable(receive_processor cb) {
    if (enabled) {
        // Force cache consistency. 
        // 
        // Per datasheet: Inserts a DSB instruction in to the code path. The DSB operation 
        // completes when all explicit memory accesses before this instruction complete.
        __dsb();
        // Get the DMA live write pointer (will be moving)
        // NB: Please see: https://github.com/raspberrypi/pico-feedback/issues/208
        // There were some problems in the RP2040, but testing shows that things are fine
        // on the RP2040.
        const uint8_t* dmaWritePtr = (const uint8_t* )dma_hw->ch[dma_ch_rx].write_addr;
        // This will move the nextReadPtr forward as bytes are consumed
        processRxBuf(rx_buf, &nextReadPtr, dmaWritePtr, UART_RX_BUF_SIZE,
            // This callback is fired for each **complete** message pulled from the circular
            // buffer. The header byte (0) is not included.
            [cb](const uint8_t* encodedBuf, unsigned encodedBufLen) {
                assert(encodedBufLen == FIXED_MESSAGE_SIZE - 1);
                // Decode the COBS message. Don't need space for the COBS overhead.
                uint8_t decodedBuf[FIXED_MESSAGE_SIZE - 2];
                int rc2 = cobsDecode(encodedBuf, encodedBufLen, decodedBuf, sizeof(decodedBuf));
                if (rc2 == encodedBufLen - 1)
                    cb(decodedBuf, encodedBufLen - 1);
            }
        );
    }
}

void networkAudioSend(const uint8_t* frame, unsigned len) { 
    assert(len == FIXED_MESSAGE_SIZE - 2);
    if (enabled) {
        // Header byte
        tx_buf[0] = 0;
        // Encode the message in COBS format
        cobsEncode(frame, len, tx_buf + 1, UART_TX_BUF_SIZE - 1); 
        // Stream out immediately
        dma_channel_set_read_addr(dma_ch_tx, tx_buf, false);
        dma_channel_set_trans_count(dma_ch_tx, FIXED_MESSAGE_SIZE, true);
    }
}

}
