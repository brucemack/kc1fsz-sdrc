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

#include <functional>
#include <iostream>

#include "kc1fsz-tools/CobsCodec.h"
#include "i2s_setup.h"

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

static void __not_in_flash_func(dma_tx_handler)() {
}

// IMPORTANT NOTE: This is running in an interrupt so move quickly!!!
static void __not_in_flash_func(dma_irq1_handler)() {   
    if (dma_channel_get_irq1_status(dma_ch_tx)) {
        dma_channel_acknowledge_irq1(dma_ch_tx);
        dma_tx_handler();
    }
}

static void streaming_uart_setup() {

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
        // Full size of receive buffer.
        UART_RX_BUF_SIZE,
        // Not enabled yet
        false);

    dma_ch_tx = dma_claim_unused_channel(true);
    dma_channel_config c_tx = dma_channel_get_default_config(dma_ch_tx);
    channel_config_set_transfer_data_size(&c_tx, DMA_SIZE_8);
    // Always write to static UART register
    channel_config_set_write_increment(&c_tx, false); 
    // Reading from increasing memory address    
    channel_config_set_read_increment(&c_tx, true); 
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
    dma_channel_set_irq1_enabled(dma_ch_tx, true);

    // Bind to the interrupt handler 
    irq_set_exclusive_handler(DMA_IRQ_1, dma_irq1_handler);
    irq_set_enabled(DMA_IRQ_1, true);
    
    // Start the ball rolling on the RX side
    dma_channel_start(dma_ch_rx);
}


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

void networkAudioSend(const uint8_t* frame, unsigned len) { 
    assert(len == 128 + 4);
    if (dma_ch_tx) {
        tx_buf[0] = 0;
        tx_buf[1] = 1;
        mempcy(tx_buf + 2, frame, len);
        dma_channel_set_read_addr(dma_ch_tx, tx_buf, false);
        dma_channel_set_trans_count(dma_ch_tx, 134, true);
    }
}

}
