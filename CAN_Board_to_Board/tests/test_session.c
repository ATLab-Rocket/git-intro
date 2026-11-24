#include "atlab_session.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    ATLAB_Session s = {0};
    ATLAB_CAN_Frame f;
    for (unsigned seq = 0; seq < 768U; seq++) {
        ATLAB_Session_Sent(&s, (uint8_t)seq, seq * 1000U);
        ATLAB_CAN_MakeAck(&f, (uint8_t)seq);
        assert(ATLAB_Session_Ack(&s, &f, seq * 1000U + 249U));
        assert(!s.pending && s.next == (uint8_t)(seq + 1U));
        assert(ATLAB_Session_Hello(&s, (uint8_t)seq));
        assert(!ATLAB_Session_Ack(&s, &f, seq * 1000U + 249U)); /* duplicate */
    }
    assert(s.ack_ok == 768U && s.invalid_ack == 768U);
    ATLAB_Session_Sent(&s, 17U, 100U);
    ATLAB_CAN_MakeAck(&f, 16U);
    assert(!ATLAB_Session_Ack(&s, &f, 101U) && s.pending); /* stale */
    ATLAB_CAN_MakeHello(&f, 17U);
    assert(!ATLAB_Session_Ack(&s, &f, 102U) && s.pending); /* wrong type */
    ATLAB_CAN_MakeAck(&f, 17U);
    assert(!ATLAB_Session_Ack(&s, &f, 350U) && !s.pending && s.timeout == 1U);
    assert(!ATLAB_Session_Expire(&s, 351U)); /* count exactly once */
    ATLAB_Session_Sent(&s, 255U, UINT32_MAX - 100U);
    assert(!ATLAB_Session_Expire(&s, 148U)); /* 249 ms across tick wrap */
    assert(ATLAB_Session_Expire(&s, 149U));
    assert(s.next == 0U);
    assert(!ATLAB_Session_Hello(&s, 1U)); /* gap, resync */
    assert(ATLAB_Session_Hello(&s, 2U));
    assert(!ATLAB_Session_Hello(&s, 2U)); /* duplicate */
    puts("session: PASS (768 exchanges, wrap, stale/duplicate/wrong/late ACK, timeout, gaps)");
}
