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

    puts("ATlab CAN protocol tests: PASS (software-only)");
    return 0;
}
