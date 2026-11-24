#include "atlab_can_hal.h"
#include <string.h>

/* Manual GPIO/peripheral clock setup: no CAN pin setting in .ioc required.
 * The ST HAL CAN driver still needs to be present in the build.
 */
HAL_StatusTypeDef ATLAB_CAN_BringUp(CAN_HandleTypeDef *hcan, uint8_t loopback)
{
    GPIO_InitTypeDef gpio = {0};
    CAN_FilterTypeDef filter = {0};

    if (hcan == NULL || HAL_RCC_GetPCLK1Freq() != 16000000U) return HAL_ERROR;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_CAN1_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOB, &gpio);

    memset(hcan, 0, sizeof(*hcan));
    hcan->Instance = CAN1;
    hcan->Init.Prescaler = 2U;
    hcan->Init.Mode = loopback ? CAN_MODE_LOOPBACK : CAN_MODE_NORMAL;
    hcan->Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan->Init.TimeSeg1 = CAN_BS1_13TQ;
    hcan->Init.TimeSeg2 = CAN_BS2_2TQ;
    hcan->Init.TimeTriggeredMode = DISABLE;
    hcan->Init.AutoBusOff = DISABLE;
    hcan->Init.AutoWakeUp = DISABLE;
    hcan->Init.AutoRetransmission = ENABLE;
    hcan->Init.ReceiveFifoLocked = DISABLE;
    hcan->Init.TransmitFifoPriority = DISABLE;

    if (HAL_CAN_Init(hcan) != HAL_OK) return HAL_ERROR;

    /* Accept all standard IDs; tighten the mask for production systems. */
    filter.FilterBank = 0U;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0U;
    filter.FilterIdLow = 0U;
    filter.FilterMaskIdHigh = 0U;
    filter.FilterMaskIdLow = 0U;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14U;

    if (HAL_CAN_ConfigFilter(hcan, &filter) != HAL_OK) return HAL_ERROR;
    return HAL_CAN_Start(hcan);
}

HAL_StatusTypeDef ATLAB_CAN_Send(CAN_HandleTypeDef *hcan,
                               const ATLAB_CAN_Frame *frame)
{
    CAN_TxHeaderTypeDef header = {0};
    uint32_t mailbox = 0U;

    if (hcan == NULL || frame == NULL ||
        frame->dlc > ATLAB_CAN_MAX_DLC ||
        frame->standard_id > 0x7FFU) return HAL_ERROR;
    if (HAL_CAN_GetTxMailboxesFreeLevel(hcan) == 0U) return HAL_BUSY;

    header.StdId = frame->standard_id;
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.DLC = frame->dlc;
    header.TransmitGlobalTime = DISABLE;
    return HAL_CAN_AddTxMessage(hcan, &header, (uint8_t *)frame->data, &mailbox);
}

HAL_StatusTypeDef ATLAB_CAN_TryReceive(CAN_HandleTypeDef *hcan,
                                     ATLAB_CAN_Frame *frame,
                                     uint8_t *got_frame)
{
    CAN_RxHeaderTypeDef header = {0};
    uint8_t bytes[8] = {0};
    HAL_StatusTypeDef status;

    if (hcan == NULL || frame == NULL || got_frame == NULL) return HAL_ERROR;
    *got_frame = 0U;
    if (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) == 0U) return HAL_OK;

    status = HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &header, bytes);
    if (status != HAL_OK) return status;
    if (header.IDE != CAN_ID_STD || header.RTR != CAN_RTR_DATA ||
        header.DLC > ATLAB_CAN_MAX_DLC) return HAL_ERROR;
    memset(frame, 0, sizeof(*frame));
    frame->standard_id = (uint16_t)header.StdId;
    frame->dlc = (uint8_t)header.DLC;
    memcpy(frame->data, bytes, frame->dlc);
    *got_frame = 1U;
    return HAL_OK;
}
