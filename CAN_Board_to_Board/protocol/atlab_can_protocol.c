#include "atlab_can_protocol.h"
#include <stddef.h>
#include <string.h>

void ATLAB_CAN_MakeHello(ATLAB_CAN_Frame *frame, uint8_t sequence)
{
    if (frame == NULL) return;
    memset(frame, 0, sizeof(*frame));
    frame->standard_id = ATLAB_CAN_HELLO_ID;
    frame->dlc = 6U;
    frame->data[0] = 'H';
    frame->data[1] = 'E';
    frame->data[2] = 'L';
    frame->data[3] = 'L';
    frame->data[4] = 'O';
    frame->data[5] = sequence;
}

void ATLAB_CAN_MakeAck(ATLAB_CAN_Frame *frame, uint8_t sequence)
{
    if (frame == NULL) return;
    memset(frame, 0, sizeof(*frame));
    frame->standard_id = ATLAB_CAN_ACK_ID;
    frame->dlc = 4U;
    frame->data[0] = 'A';
    frame->data[1] = 'C';
    frame->data[2] = 'K';
    frame->data[3] = sequence;
}

int ATLAB_CAN_ParseHello(const ATLAB_CAN_Frame *frame, uint8_t *sequence)
{
    if (frame == NULL || sequence == NULL ||
        frame->standard_id != ATLAB_CAN_HELLO_ID || frame->dlc != 6U ||
        memcmp(frame->data, "HELLO", 5U) != 0) return 0;
    *sequence = frame->data[5];
    return 1;
}

int ATLAB_CAN_ParseAck(const ATLAB_CAN_Frame *frame, uint8_t *sequence)
{
    if (frame == NULL || sequence == NULL ||
        frame->standard_id != ATLAB_CAN_ACK_ID || frame->dlc != 4U ||
        memcmp(frame->data, "ACK", 3U) != 0) return 0;
    *sequence = frame->data[3];
    return 1;
}
