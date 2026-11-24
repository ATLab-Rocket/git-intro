#include "atlab_demo.h"
#include "atlab_can_hal.h"
#include "atlab_debug_log.h"
#ifndef ATLAB_CAN_TEST_LOOPBACK
#error "Choose Normal or Loopback build configuration"
#endif
#if defined(STM32F407xx)
#define DEVICE "F407"
#define INITIATOR 1
#else
#define DEVICE "L476"
#define INITIATOR ATLAB_CAN_TEST_LOOPBACK
#endif
extern CAN_HandleTypeDef hcan1;
ATLAB_Session atlab_session;
volatile uint32_t atlab_rx_ok, atlab_bad_frame;
static int send_frame(const ATLAB_CAN_Frame *f)
{
    if (ATLAB_CAN_Send(&hcan1, f) != HAL_OK) {
        ATLAB_CAN_RecordError();
        ATLAB_Log("CAN TX reject ID=%03X\r\n", (unsigned)f->standard_id);
        return 0;
    }
    ATLAB_Log("CAN TX queued ID=%03X seq=%u\r\n", (unsigned)f->standard_id, (unsigned)f->data[f->dlc-1U]);
    return 1;
}
void ATLAB_Demo_Run(void)
{
    uint32_t ping = HAL_GetTick(), heartbeat = ping;
    if (ATLAB_CAN_BringUp(&hcan1, ATLAB_CAN_TEST_LOOPBACK) != HAL_OK) {
        ATLAB_CAN_RecordError();
        for (;;) { ATLAB_Log_Service(); }
    }
    ATLAB_Log(DEVICE " mode=%s PCLK1=%lu prescaler=%lu CAN=500000\r\n",
        ATLAB_CAN_TEST_LOOPBACK ? "SILENT_LOOPBACK" : "NORMAL",
        (unsigned long)atlab_can_pclk1, (unsigned long)atlab_can_prescaler);
    for (;;) {
        uint32_t now = HAL_GetTick();
        ATLAB_CAN_Service(&hcan1, now);
        if (ATLAB_Session_Expire(&atlab_session, now))
            ATLAB_Log("ACK TIMEOUT seq=%u\r\n", (unsigned)atlab_session.expected);
        if ((uint32_t)(now - ping) >= 1000U) {
            ping = now;
#if INITIATOR
            ATLAB_CAN_Frame hello;
            ATLAB_CAN_MakeHello(&hello, atlab_session.next);
            if (send_frame(&hello)) ATLAB_Session_Sent(&atlab_session, atlab_session.next, now);
#endif
        }
        /* Bound work per pass so an RX flood cannot starve logging or timeout. */
        for (unsigned n = 0U; n < 8U; n++) {
            ATLAB_CAN_Frame f;
            uint8_t got, seq;
            if (ATLAB_CAN_TryReceive(&hcan1, &f, &got) != HAL_OK) { ATLAB_CAN_RecordError(); break; }
            if (!got) break;
            atlab_rx_ok++;
            ATLAB_Log("CAN RX ID=%03X DLC=%u\r\n", (unsigned)f.standard_id, (unsigned)f.dlc);
            if (ATLAB_CAN_ParseHello(&f, &seq)) {
#if defined(STM32L476xx) || ATLAB_CAN_TEST_LOOPBACK
                if (!ATLAB_Session_Hello(&atlab_session, seq))
                    ATLAB_Log("HELLO sequence error seq=%u\r\n", (unsigned)seq);
                ATLAB_CAN_Frame ack;
                ATLAB_CAN_MakeAck(&ack, seq);
                (void)send_frame(&ack);
#else
                atlab_bad_frame++;
#endif
            } else if (ATLAB_CAN_ParseAck(&f, &seq)) {
#if INITIATOR
                int valid = ATLAB_Session_Ack(&atlab_session, &f, HAL_GetTick());
                ATLAB_Log("ACK %s seq=%u\r\n", valid ? "OK" : "INVALID", (unsigned)seq);
#else
                atlab_bad_frame++;
#endif
            } else {
                atlab_bad_frame++;
                ATLAB_Log("CAN RX invalid packet\r\n");
            }
        }
        if ((uint32_t)(now - heartbeat) >= 1000U) {
            heartbeat = now;
            ATLAB_Log(DEVICE " Heartbeat seq=%u TX=%lu RX=%lu ACK=%lu TO=%lu ERR=%lu BAD=%lu DROP=%lu\r\n",
                (unsigned)atlab_session.next, (unsigned long)atlab_tx_ok, (unsigned long)atlab_rx_ok,
                (unsigned long)atlab_session.ack_ok, (unsigned long)atlab_session.timeout,
                (unsigned long)atlab_can_errors, (unsigned long)(atlab_bad_frame + atlab_session.invalid_ack + atlab_session.sequence_error),
                (unsigned long)atlab_log_drops);
        }
        ATLAB_Log_Service();
    }
}
