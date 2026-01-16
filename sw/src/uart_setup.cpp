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

// 3rd party
#include "cobs.h"

#include "uart_setup.h"

using namespace std;

namespace kc1fsz {

#define NETWORK_BAUD (1152000)
#define COBS_OVERHEAD (2)
// Fixed size, 1 header null + audio + 1 COBS overhead
#define NETWORK_MESSAGE_SIZE (1 + (160 * 2) + COBS_OVERHEAD)

#define UART_RX_BUF_SIZE 512
#define UART_RX_BUF_BITS (9)
// This is one more bit since the counter gets to 1000..00
#define UART_RX_BUF_MASK (0b1111111111)

//#define UART_RX_BUF_SIZE 1024
//#define UART_RX_BUF_BITS (10)
// This is one more bit since the counter gets to 1000..00
//#define UART_RX_BUF_MASK (0b11111111111)

#define UART_TX_BUF_SIZE 512
#define UART_TX_BUF_BITS (9)
// This is one more bit since the counter gets to 1000..00
#define UART_TX_BUF_MASK (0b1111111111)
#define UART_TX_BUF_MASK2 (0b111111111)

#define HEADER_CODE (0)

static bool enabled = false;

// Receive Related:

static uint dma_ch_rx = 0;
// This buffer is used with the DMA ring feature so it needs to be power-of-two
// aligned.
static uint8_t __attribute__((aligned(UART_RX_BUF_SIZE))) rx_buf[UART_RX_BUF_SIZE];

// This pointer keeps track of read progress around the circular receive buffer
static unsigned nextReadPtr = 0;
// Accumulator for a received message
static uint8_t completeMsg[NETWORK_MESSAGE_SIZE];
static unsigned completeMsgLen = 0;
static bool haveHeader = false;
static unsigned ReceiveCount = 0;

// Transmit Related:

static uint dma_ch_tx = 0;
// This buffer is used with the DMA ring feature so it needs to be power-of-two
// aligned.
static uint8_t __attribute__((aligned(UART_TX_BUF_SIZE))) TxBuf[UART_TX_BUF_SIZE];
static unsigned TxBufRdPtr = 0;
static unsigned TxBufWrPtr = 0;
static unsigned TxBufLen = 0;
static bool TxDmaInProcess = false;
static unsigned TxDmaLength = 0;

static unsigned TxBufOverflow = 0;
static unsigned OverlapSendDiscardedCount = 0;

/*
static void __not_in_flash_func(dma_tx_handler)() {
}

// IMPORTANT NOTE: This is running in an interrupt so move quickly!!!
static void __not_in_flash_func(dma_irq1_handler)() {   
    if (dma_channel_get_irq1_status(dma_ch_tx)) {
        dma_channel_acknowledge_irq1(dma_ch_tx);
        dma_tx_handler();
    }
}
*/

void streaming_uart_setup() {

    uart_init(uart0, NETWORK_BAUD);
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
    channel_config_set_ring(&c_rx, true, UART_RX_BUF_BITS);
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
    // Circular ring buffer. The "false" means that this applies
    // to the read address, which is what is relevant for sending
    // data to the UART.
    channel_config_set_ring(&c_tx, false, UART_TX_BUF_BITS);
    channel_config_set_enable(&c_tx, true);
    
    dma_channel_configure(
        dma_ch_tx,
        &c_tx,
        &uart_get_hw(uart0)->dr,
        TxBuf,
        UART_TX_BUF_SIZE,
        // Not started yet
        false);
    // The TX channel should fire an interrupt whenever it is complete
    //dma_channel_set_irq1_enabled(dma_ch_tx, true);

    // Bind to the interrupt handler 
    //irq_set_exclusive_handler(DMA_IRQ_1, dma_irq1_handler);
    //irq_set_enabled(DMA_IRQ_1, true);        

    // Start the ball rolling on the RX side
    dma_channel_start(dma_ch_rx);
    enabled = true;
}

/** 
 * @brief Puts outbound data into a circular transmit queue for later processing
 * by the DMA system.
 * 
 * @returns 0 If the entire message was accepted
 */
static int queueForTx(const uint8_t* data, unsigned len) {
    unsigned spaceAvailable = UART_TX_BUF_SIZE - TxBufLen;
    if (len > spaceAvailable)
        return -1;
    for (unsigned i = 0; i < len; i++) {
        TxBuf[TxBufWrPtr] = data[i];
        // NOTE: It's very important that the write pointer and the length
        // be updated in tandem.
        TxBufWrPtr = (TxBufWrPtr + 1) & UART_TX_BUF_MASK2;
        TxBufLen++;
    }
    return 0;
}

// Should be called whenever data is queued and on every tick to keep pushing
// transmit data into the DMA system.
static void startTxDMAIfPossible() {

    bool dmaRunning = (dma_hw->ch[dma_ch_tx].transfer_count & UART_TX_BUF_MASK) != 0;
    // Look to see if we just finished a DMA transfer
    if (TxDmaInProcess && !dmaRunning) {
        // Move our version of the read pointer forward to prepare for next time. 
        // This pointer should track the DMA read address.
        // NOTE: It's very important that the read pointer and the length
        // be updated in tandem.
        TxBufRdPtr = (TxBufRdPtr + TxDmaLength) & UART_TX_BUF_MASK2;
        TxBufLen -= TxDmaLength;
        TxDmaInProcess = false;
        TxDmaLength = 0;
    }
    // Check to make sure there isn't already a TX in process. 
    if (dmaRunning) {
        OverlapSendDiscardedCount++;
        return;
    }
    // Anything pending?
    if (TxBufLen == 0)
        return;
    // Trigger the DMA. Keep in mind that we are configured in DMA ring mode
    // so the wrap around the end of the buffer will happen automatically.
    dma_channel_set_read_addr(dma_ch_tx, TxBuf + TxBufRdPtr, false);
    dma_channel_set_trans_count(dma_ch_tx, TxBufLen, false);
    dma_channel_start(dma_ch_tx);
    TxDmaInProcess = true;
    TxDmaLength = TxBufLen;
}

unsigned count2 = 0;

static void processRxBuf(unsigned nextWritePtr,
    std::function<void(const uint8_t* buf, unsigned bufLen)> cb) {

    bool firedCb = false;

    while (!firedCb && nextReadPtr != nextWritePtr) {
        assert(nextReadPtr < UART_RX_BUF_SIZE);
        // When a null is found we reset accumulation
        if (rx_buf[nextReadPtr] == HEADER_CODE) {
            completeMsg[0] = 0;
            completeMsgLen = 1;
            haveHeader = true;
        } else if (haveHeader) {
            // Accumulating a potential message
            completeMsg[completeMsgLen++] = rx_buf[nextReadPtr];
            // Did we get a full message?
            if (completeMsgLen == NETWORK_MESSAGE_SIZE) {
                // ##### TODO CHECKSUM VALIDATION
                // Fire the callback and give the encoded message, minus the 
                // header byte.
                cb(completeMsg + 1, completeMsgLen - 1);
                firedCb = true;
                haveHeader = false;
            }
        } 
        // Increment and wrap
        nextReadPtr++;
        if (nextReadPtr == UART_RX_BUF_SIZE) 
            nextReadPtr = 0;
    }
}

void networkAudioReceiveIfAvailable(receive_processor cb) {

    if (!enabled) 
        return;

    // Force cache consistency. 
    // 
    // Per datasheet: Inserts a DSB instruction in to the code path. The DSB operation 
    // completes when all explicit memory accesses before this instruction complete.
    __dsb();

    // Get the DMA live write pointer (will be moving continuously)
    // Here is what we'd like to do:
    //
    //    const uint8_t* dmaWritePtr = (const uint8_t* )dma_hw->ch[dma_ch_rx].write_addr;
    //
    // But NB: Please see: https://github.com/raspberrypi/pico-feedback/issues/208
    // There were some problems in the RP2040/RP2350. (RP2040-E12).  Basically, the 
    // WRITE_ADDR can get ahead of reality in the ring case (that we are using).
    //
    // So here is what we need to do to work around the defect. The top bits of the 
    // TRANS_COUNT have special meaning on the RP2350 so we only focus on the low end 
    // of the counter.
    unsigned transferCountRemaining = (dma_hw->ch[dma_ch_rx].transfer_count & UART_RX_BUF_MASK);
    assert(transferCountRemaining <= UART_RX_BUF_SIZE);
    unsigned dmaWritePtr = UART_RX_BUF_SIZE - transferCountRemaining;
    // Wrap in the case where the DMA and gone all the way to the end
    if (dmaWritePtr == UART_RX_BUF_SIZE)
        dmaWritePtr = 0;

    // Process the received bytes if possible. This will move the nextReadPtr forward 
    // as bytes are consumed.
    processRxBuf(dmaWritePtr,
        // This callback is fired for each **complete** message pulled from the circular
        // buffer. The header byte is NOT included.
        [cb](const uint8_t* encodedBuf, unsigned encodedBufLen) {

            assert(encodedBufLen == NETWORK_MESSAGE_SIZE - 1);
            ReceiveCount++;

            // Decode the COBS message. Don't need space for the COBS overhead.
            uint8_t decodedBuf[NETWORK_MESSAGE_SIZE - 1 - COBS_OVERHEAD];
            cobs_decode_result rd = cobs_decode(decodedBuf, sizeof(decodedBuf), 
                encodedBuf, encodedBufLen);
            assert(rd.status == COBS_DECODE_OK);  
            assert(rd.out_len == sizeof(decodedBuf));
            cb(decodedBuf, sizeof(decodedBuf));

            /*
            // #### TEMP ECHO OF ENCODED MESSAGE
            // Make a complete message
            uint8_t txMsg[NETWORK_MESSAGE_SIZE];
            txMsg[0] = 0;
            memcpy(txMsg + 1, encodedBuf, encodedBufLen);
            txMsg[1] = TxBufOverflow;
            txMsg[2] = ReceiveCount;
            txMsg[NETWORK_MESSAGE_SIZE - 1] = 'z';
            // NOTE: This is an all-or-nothing queue to avoid half-baked
            // messages going out on the network.
            int rc = queueForTx(txMsg, NETWORK_MESSAGE_SIZE);
            if (rc != 0)
                TxBufOverflow++;
            */
        }
    );

    // Check if there is anything waiting to go out
    startTxDMAIfPossible();
}

void networkAudioSend(const uint8_t* frame, unsigned len) { 
    if (enabled) {
        assert(len == NETWORK_MESSAGE_SIZE - 1 - COBS_OVERHEAD);
        // Check to make sure there is already a send in process. If so, 
        // the send is lost. 
        // NOTE: I've seen cases where two transmits overlapped
        unsigned transferCountRemaining = dma_hw->ch[dma_ch_tx].transfer_count;
        if (transferCountRemaining != 0) {
            OverlapSendDiscardedCount++;
            return;
        }
        // Header byte
        uint8_t txFrame[NETWORK_MESSAGE_SIZE];
        txFrame[0] = HEADER_CODE;       
        // Encode the message in COBS format
        cobs_encode_result re = cobs_encode(txFrame + 1, NETWORK_MESSAGE_SIZE - 1, 
            frame, len);
        assert(re.status == COBS_ENCODE_OK);
        assert(re.out_len <= len + COBS_OVERHEAD);
        queueForTx(txFrame, NETWORK_MESSAGE_SIZE);
    }
}

}
