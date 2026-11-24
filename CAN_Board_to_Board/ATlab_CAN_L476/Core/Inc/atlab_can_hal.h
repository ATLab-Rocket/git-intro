#ifndef ATLAB_CAN_HAL_H
#define ATLAB_CAN_HAL_H
#include "main.h"
#include "atlab_can_protocol.h"
HAL_StatusTypeDef ATLAB_CAN_BringUp(CAN_HandleTypeDef *hcan, uint8_t loopback);
HAL_StatusTypeDef ATLAB_CAN_Send(CAN_HandleTypeDef *hcan, const ATLAB_CAN_Frame *frame);
HAL_StatusTypeDef ATLAB_CAN_TryReceive(CAN_HandleTypeDef *hcan, ATLAB_CAN_Frame *frame, uint8_t *got_frame);
void ATLAB_CAN_RecordError(void);
void ATLAB_CAN_Service(CAN_HandleTypeDef *hcan, uint32_t now);
extern volatile uint32_t atlab_tx_ok, atlab_tx_queued, atlab_tx_timeout, atlab_can_errors;
extern uint32_t atlab_can_pclk1, atlab_can_prescaler;
#endif
