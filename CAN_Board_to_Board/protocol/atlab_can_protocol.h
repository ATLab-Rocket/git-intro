#ifndef ATLAB_CAN_PROTOCOL_H
#define ATLAB_CAN_PROTOCOL_H

/* Classic CAN, standard 11-bit identifiers, at most 8 data bytes. */
#include <stdint.h>

#define ATLAB_CAN_HELLO_ID 0x101U
#define ATLAB_CAN_ACK_ID   0x102U
#define ATLAB_CAN_MAX_DLC  8U

typedef struct {
    uint16_t standard_id;
    uint8_t dlc;
    uint8_t data[ATLAB_CAN_MAX_DLC];
} ATLAB_CAN_Frame;

void ATLAB_CAN_MakeHello(ATLAB_CAN_Frame *frame, uint8_t sequence);
void ATLAB_CAN_MakeAck(ATLAB_CAN_Frame *frame, uint8_t sequence);
int ATLAB_CAN_ParseHello(const ATLAB_CAN_Frame *frame, uint8_t *sequence);
int ATLAB_CAN_ParseAck(const ATLAB_CAN_Frame *frame, uint8_t *sequence);

#endif
