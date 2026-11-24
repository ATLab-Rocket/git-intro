#include "atlab_demo.h"
#include "atlab_can_hal.h"
#include "atlab_can_protocol.h"
#include "atlab_debug_log.h"

/* Set to 1 for each-board self-loopback test, 0 for real CAN bus next week.
 * In loopback both boards echo only their own CAN frames, NOT each other.
 */
#ifndef ATLAB_CAN_TEST_LOOPBACK
#define ATLAB_CAN_TEST_LOOPBACK 1
#endif

#if !defined(STM32F407xx) && !defined(STM32L476xx)
#error "Target must be STM32F407xx or STM32L476xx"
#endif

#if defined(STM32F407xx)
#define ATLAB_DEVICE_NAME "F407"
#else
#define ATLAB_DEVICE_NAME "L476"
#endif

volatile uint32_t atlab_tx_ok = 0U;
volatile uint32_t atlab_rx_ok = 0U;
volatile uint32_t atlab_ack_ok = 0U;
volatile uint32_t atlab_can_errors = 0U;

static void send_frame(CAN_HandleTypeDef *hcan,
                       const ATLAB_CAN_Frame *frame)
{
    if (ATLAB_CAN_Send(hcan, frame) == HAL_OK) {
        atlab_tx_ok++;
        ATLAB_Log("[%s] TX ID=0x%03X seq=%u\r\n", ATLAB_DEVICE_NAME,
                  frame->standard_id, frame->data[frame->dlc - 1U]);
    } else {
        atlab_can_errors++;
        ATLAB_Log("[%s] TX ERROR\r\n", ATLAB_DEVICE_NAME);
    }
}

void ATLAB_Demo_Run(void)
{
    CAN_HandleTypeDef hcan1 = {0};
    uint32_t next_ping_ms;
    uint8_t next_sequence = 0U;

    if (ATLAB_CAN_BringUp(&hcan1, ATLAB_CAN_TEST_LOOPBACK) != HAL_OK) {
        atlab_can_errors++;
        ATLAB_Log("[%s] CAN init error: check APB1=16 MHz and HAL CAN\r\n",
                  ATLAB_DEVICE_NAME);
        for (;;) HAL_Delay(1000U);
    }
    ATLAB_Log("[%s] CAN started mode=%s bitrate=500k\r\n",
              ATLAB_DEVICE_NAME,
              ATLAB_CAN_TEST_LOOPBACK ? "LOOPBACK" : "NORMAL");
    next_ping_ms = HAL_GetTick();

    for (;;) {
        ATLAB_CAN_Frame incoming = {0};
        uint8_t received = 0U;
        uint8_t sequence = 0U;
        uint32_t now = HAL_GetTick();

        if ((int32_t)(now - next_ping_ms) >= 0) {
            next_ping_ms = now + 1000U;
#if defined(STM32F407xx)
            ATLAB_CAN_Frame outgoing;
            ATLAB_CAN_MakeHello(&outgoing, next_sequence++);
            send_frame(&hcan1, &outgoing);
#elif ATLAB_CAN_TEST_LOOPBACK
            /* L476 transmits its own HELLO in loopback for standalone test. */
            ATLAB_CAN_Frame outgoing;
            ATLAB_CAN_MakeHello(&outgoing, next_sequence++);
            send_frame(&hcan1, &outgoing);
#else
            ATLAB_Log("[L476] waiting for HELLO\r\n");
#endif
        }

        if (ATLAB_CAN_TryReceive(&hcan1, &incoming, &received) != HAL_OK) {
            atlab_can_errors++;
            ATLAB_Log("[%s] RX ERROR\r\n", ATLAB_DEVICE_NAME);
        } else if (received) {
            atlab_rx_ok++;
            if (ATLAB_CAN_ParseHello(&incoming, &sequence)) {
                ATLAB_Log("[%s] RX HELLO seq=%u\r\n",
                          ATLAB_DEVICE_NAME, sequence);
#if defined(STM32L476xx) && !ATLAB_CAN_TEST_LOOPBACK
                ATLAB_CAN_Frame ack;
                ATLAB_CAN_MakeAck(&ack, sequence);
                send_frame(&hcan1, &ack);
#endif
            } else if (ATLAB_CAN_ParseAck(&incoming, &sequence)) {
                atlab_ack_ok++;
                ATLAB_Log("[%s] RX ACK seq=%u\r\n",
                          ATLAB_DEVICE_NAME, sequence);
            } else {
                ATLAB_Log("[%s] RX other ID=0x%03X DLC=%u\r\n",
                          ATLAB_DEVICE_NAME,
                          incoming.standard_id, incoming.dlc);
            }
        }
        HAL_Delay(5U);
    }
}
