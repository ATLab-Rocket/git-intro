#include "atlab_can_protocol.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    ATLAB_CAN_Frame ping = {0};
    ATLAB_CAN_Frame ack = {0};
    uint8_t seq = 0U;

    for (unsigned i = 0; i < 256U; i++) {
        ATLAB_CAN_MakeHello(&ping, (uint8_t)i);
        assert(ping.standard_id == ATLAB_CAN_HELLO_ID && ping.dlc == 6U);
        assert(ATLAB_CAN_ParseHello(&ping, &seq) == 1 && seq == (uint8_t)i);
        assert(ATLAB_CAN_ParseAck(&ping, &seq) == 0);

        ATLAB_CAN_MakeAck(&ack, (uint8_t)i);
        assert(ack.standard_id == ATLAB_CAN_ACK_ID && ack.dlc == 4U);
        assert(ATLAB_CAN_ParseAck(&ack, &seq) == 1 && seq == (uint8_t)i);
        assert(ATLAB_CAN_ParseHello(&ack, &seq) == 0);
    }

    ATLAB_CAN_MakeHello(&ping, 42U);
    ping.data[2] = 'X';
    assert(ATLAB_CAN_ParseHello(&ping, &seq) == 0);
    ATLAB_CAN_MakeAck(&ack, 42U);
    ack.dlc = 8U;
    assert(ATLAB_CAN_ParseAck(&ack, &seq) == 0);
    assert(ATLAB_CAN_ParseHello(NULL, &seq) == 0);
    assert(ATLAB_CAN_ParseAck(&ack, NULL) == 0);

    /* Exhaustively reject every other 11-bit ID and every incorrect DLC. */
    for (unsigned id = 0U; id < 2048U; id++) {
        ATLAB_CAN_MakeHello(&ping, 42U); ping.standard_id = (uint16_t)id;
        assert(ATLAB_CAN_ParseHello(&ping, &seq) == (id == ATLAB_CAN_HELLO_ID));
        ATLAB_CAN_MakeAck(&ack, 42U); ack.standard_id = (uint16_t)id;
        assert(ATLAB_CAN_ParseAck(&ack, &seq) == (id == ATLAB_CAN_ACK_ID));
    }
    for (unsigned dlc = 0U; dlc < 256U; dlc++) {
        ATLAB_CAN_MakeHello(&ping, 42U); ping.dlc = (uint8_t)dlc;
        assert(ATLAB_CAN_ParseHello(&ping, &seq) == (dlc == 6U));
        ATLAB_CAN_MakeAck(&ack, 42U); ack.dlc = (uint8_t)dlc;
        assert(ATLAB_CAN_ParseAck(&ack, &seq) == (dlc == 4U));
    }
    for (unsigned pos = 0U; pos < 5U; pos++) {
        ATLAB_CAN_MakeHello(&ping, 42U); ping.data[pos] ^= 1U;
        seq = 99U;
        assert(!ATLAB_CAN_ParseHello(&ping, &seq) && seq == 99U);
    }
    for (unsigned pos = 0U; pos < 3U; pos++) {
        ATLAB_CAN_MakeAck(&ack, 42U); ack.data[pos] ^= 1U;
        assert(!ATLAB_CAN_ParseAck(&ack, &seq));
    }
    ATLAB_CAN_MakeHello(NULL, 0U); ATLAB_CAN_MakeAck(NULL, 0U);
    assert(!ATLAB_CAN_ParseHello(&ping, NULL));
    assert(!ATLAB_CAN_ParseAck(NULL, &seq));
    puts("protocol: PASS (256 sequences, all 2048 IDs and 256 DLCs, payload corruption, nulls)");
    return 0;
}
