#ifndef ATLAB_SESSION_H
#define ATLAB_SESSION_H
#include "atlab_can_protocol.h"
#define ATLAB_ACK_TIMEOUT_MS 250U
typedef struct {
    uint32_t sent_at;
    uint32_t ack_ok, timeout, invalid_ack, sequence_error;
    uint8_t pending, expected, next, rx_seen, rx_last;
} ATLAB_Session;
/* Main-loop only. Register a HELLO after HAL accepts its TX request. */
void ATLAB_Session_Sent(ATLAB_Session *s, uint8_t seq, uint32_t now);
int ATLAB_Session_Ack(ATLAB_Session *s, const ATLAB_CAN_Frame *f, uint32_t now);
int ATLAB_Session_Expire(ATLAB_Session *s, uint32_t now);
/* First HELLO establishes baseline. A gap records an error and resynchronizes. */
int ATLAB_Session_Hello(ATLAB_Session *s, uint8_t seq);
#endif
