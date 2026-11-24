#ifndef ATLAB_DEMO_H
#define ATLAB_DEMO_H

#include "main.h"

/* Configure system clock to HSI 16 MHz / APB1 16 MHz first.
 * Calls a perpetual main-loop demo (no CAN cable required in loopback).
 */
void ATLAB_Demo_Run(void);

extern volatile uint32_t atlab_tx_ok;
extern volatile uint32_t atlab_rx_ok;
extern volatile uint32_t atlab_ack_ok;
extern volatile uint32_t atlab_can_errors;

#endif
