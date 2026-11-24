#include "atlab_session.h"
void ATLAB_Session_Sent(ATLAB_Session *s, uint8_t seq, uint32_t now)
{
    s->pending = 1U;
    s->expected = seq;
    s->sent_at = now;
    s->next = (uint8_t)(seq + 1U);
}
int ATLAB_Session_Expire(ATLAB_Session *s, uint32_t now)
{
    if (s->pending && (uint32_t)(now - s->sent_at) >= ATLAB_ACK_TIMEOUT_MS) {
        s->pending = 0U;
        s->timeout++;
        return 1;
    }
    return 0;
}
int ATLAB_Session_Ack(ATLAB_Session *s, const ATLAB_CAN_Frame *f, uint32_t now)
{
    uint8_t seq;
    (void)ATLAB_Session_Expire(s, now);
    if (!ATLAB_CAN_ParseAck(f, &seq) || !s->pending || seq != s->expected) {
        s->invalid_ack++;
        return 0;
    }
    s->pending = 0U;
    s->ack_ok++;
    return 1;
}
int ATLAB_Session_Hello(ATLAB_Session *s, uint8_t seq)
{
    int valid = !s->rx_seen || seq == (uint8_t)(s->rx_last + 1U);
    if (!valid) s->sequence_error++;
    s->rx_seen = 1U;
    s->rx_last = seq;
    return valid;
}
