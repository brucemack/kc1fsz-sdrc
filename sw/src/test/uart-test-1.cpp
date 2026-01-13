#include <iostream>

#include "kc1fsz-tools/CobsCodec.h"
#include "uart_setup.h"

using namespace std;
using namespace kc1fsz;

int main(int,const char**) {

    const unsigned RX_BUF_SIZE = 512;
    uint8_t rxBuf[RX_BUF_SIZE];
    memset(rxBuf, 1, RX_BUF_SIZE);

    uint8_t* nextRd = rxBuf + 2;
    uint8_t* dmaWr = rxBuf + 16;
    // Should be no header byte
    assert(processRxBuf(rxBuf, &nextRd, dmaWr, RX_BUF_SIZE,
        [](const uint8_t*,unsigned) { }) == 1);
    assert(nextRd == dmaWr);

    // Test wrap-around case
    nextRd = rxBuf + 16;
    dmaWr = rxBuf + 2;
    // Should be no header byte
    assert(processRxBuf(rxBuf, &nextRd, dmaWr, RX_BUF_SIZE,
        [](const uint8_t*,unsigned) { }) == 1);
    assert(nextRd == dmaWr);

    // Put a partial message in. This should not advance 
    // the read pointer (still waiting)

    dmaWr = rxBuf;
    dmaWr[0] = 0;
    dmaWr++;
    uint8_t inmsg[8] = { 0, 2, 3, 0xff, 5, 6, 7, 0xff };
    int rc = cobsEncode(inmsg, 8, dmaWr, 9);
    assert(rc == 9);
    dmaWr += 9;

    nextRd = rxBuf;
    assert(processRxBuf(rxBuf, &nextRd, dmaWr, RX_BUF_SIZE,
        [](const uint8_t*,unsigned) { }) == 2);
    assert(nextRd == rxBuf);
    
    // Advance far enough to have a complete message to decode
    dmaWr += (133 - 9);
    assert(processRxBuf(rxBuf, &nextRd, dmaWr, RX_BUF_SIZE,
        [inmsg](const uint8_t* buf, unsigned bufLen) { 
            uint8_t msg[132];
            unsigned rc2 = cobsDecode(buf, bufLen, msg, 132);
            assert(rc2 == 132);
            assert(memcmp(msg, inmsg, 8) == 0);
        }) == 0);
    assert(nextRd == dmaWr);
}
