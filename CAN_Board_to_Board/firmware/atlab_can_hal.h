#ifndef ATLAB_CAN_HAL_H
#define ATLAB_CAN_HAL_H

#include "main.h"
#include "atlab_can_protocol.h"

/* Requires STM32Cube HAL CAN module and its source file in the IDE project.
 * Requires PCLK1 == 16 MHz; timing is 500 kbit/s (16MHz / (2*16TQ)).
 * GPIO PB8 = CAN1_RX, PB9 = CAN1_TX, AF9.
 */
HAL_StatusTypeDef ATLAB_CAN_BringUp(CAN_HandleTypeDef *hcan, uint8_t loopback);
HAL_StatusTypeDef ATLAB_CAN_Send(CAN_HandleTypeDef *hcan,
                               const ATLAB_CAN_Frame *frame);
HAL_StatusTypeDef ATLAB_CAN_TryReceive(CAN_HandleTypeDef *hcan,
                                     ATLAB_CAN_Frame *frame,
                                     uint8_t *got_frame);

#endif
