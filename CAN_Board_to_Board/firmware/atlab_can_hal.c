#include "atlab_can_hal.h"
volatile uint32_t atlab_tx_ok, atlab_tx_queued, atlab_tx_timeout, atlab_can_errors;
uint32_t atlab_can_pclk1, atlab_can_prescaler;
static uint32_t tx_mailbox, tx_since;
extern CAN_HandleTypeDef hcan1;
/* Shared by IRQ and foreground: preserve every error increment. */
void ATLAB_CAN_RecordError(void)
{
    uint32_t irq = __get_PRIMASK();
    __disable_irq();
    atlab_can_errors++;
    __set_PRIMASK(irq);
}

HAL_StatusTypeDef ATLAB_CAN_BringUp(CAN_HandleTypeDef *hcan, uint8_t loopback)
{
    CAN_FilterTypeDef filter = {0};
    if (hcan == NULL) return HAL_ERROR;
    /* bxCAN uses PCLK1, NOT the doubled APB timer clock. 16 TQ, 87.5% sample. */
    atlab_can_pclk1 = HAL_RCC_GetPCLK1Freq();
    if (atlab_can_pclk1 % (500000U * 16U) != 0U) return HAL_ERROR;
    atlab_can_prescaler = atlab_can_pclk1 / (500000U * 16U);
    if (atlab_can_prescaler < 1U || atlab_can_prescaler > 1024U) return HAL_ERROR;
    hcan->Init.Prescaler = atlab_can_prescaler;
    hcan->Init.Mode = loopback ? CAN_MODE_SILENT_LOOPBACK : CAN_MODE_NORMAL;
    hcan->Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan->Init.TimeSeg1 = CAN_BS1_13TQ;
    hcan->Init.TimeSeg2 = CAN_BS2_2TQ;
    hcan->Init.AutoBusOff = ENABLE;
    /* One-shot TX: failures counted; next HELLO continues after one second. */
    hcan->Init.AutoRetransmission = DISABLE;
    if (HAL_CAN_Init(hcan) != HAL_OK) return HAL_ERROR;
    /* Exact standard DATA IDs; IDE/RTR bits are zero in 32-bit ID list. */
    filter.FilterMode = CAN_FILTERMODE_IDLIST;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = ATLAB_CAN_HELLO_ID << 5;
    filter.FilterMaskIdHigh = ATLAB_CAN_ACK_ID << 5;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14U;
    if (HAL_CAN_ConfigFilter(hcan, &filter) != HAL_OK) return HAL_ERROR;
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN1_TX_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
    if (HAL_CAN_Start(hcan) != HAL_OK) return HAL_ERROR;
    return HAL_CAN_ActivateNotification(hcan, CAN_IT_TX_MAILBOX_EMPTY | CAN_IT_ERROR |
        CAN_IT_LAST_ERROR_CODE | CAN_IT_BUSOFF | CAN_IT_ERROR_PASSIVE |
        CAN_IT_ERROR_WARNING | CAN_IT_RX_FIFO0_OVERRUN);
}
HAL_StatusTypeDef ATLAB_CAN_Send(CAN_HandleTypeDef *hcan, const ATLAB_CAN_Frame *frame)
{
    CAN_TxHeaderTypeDef h = {0};
    if (hcan == NULL || frame == NULL || frame->dlc > 8U || frame->standard_id > 0x7FFU) return HAL_ERROR;
    if (HAL_CAN_IsTxMessagePending(hcan, CAN_TX_MAILBOX0 | CAN_TX_MAILBOX1 | CAN_TX_MAILBOX2)) return HAL_BUSY;
    h.StdId = frame->standard_id;
    h.IDE = CAN_ID_STD;
    h.RTR = CAN_RTR_DATA;
    h.DLC = frame->dlc;
    HAL_StatusTypeDef result = HAL_CAN_AddTxMessage(hcan, &h, (uint8_t *)frame->data, &tx_mailbox);
    if (result == HAL_OK) { tx_since = HAL_GetTick(); atlab_tx_queued++; }
    return result;
}
void ATLAB_CAN_Service(CAN_HandleTypeDef *hcan, uint32_t now)
{
    if (tx_mailbox && HAL_CAN_IsTxMessagePending(hcan, tx_mailbox) && (uint32_t)(now - tx_since) >= 100U) {
        (void)HAL_CAN_AbortTxRequest(hcan, tx_mailbox);
        atlab_tx_timeout++;
        tx_since = now;
    }
}
HAL_StatusTypeDef ATLAB_CAN_TryReceive(CAN_HandleTypeDef *hcan, ATLAB_CAN_Frame *frame, uint8_t *got_frame)
{
    CAN_RxHeaderTypeDef h = {0};
    uint8_t data[8] = {0};
    if (hcan == NULL || frame == NULL || got_frame == NULL) return HAL_ERROR;
    *got_frame = 0U;
    if (!HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0)) return HAL_OK;
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &h, data) != HAL_OK) return HAL_ERROR;
    if (h.IDE != CAN_ID_STD || h.RTR != CAN_RTR_DATA || h.DLC > 8U) return HAL_ERROR;
    frame->standard_id = (uint16_t)h.StdId;
    frame->dlc = (uint8_t)h.DLC;
    for (unsigned i = 0U; i < 8U; i++) frame->data[i] = data[i];
    *got_frame = 1U;
    return HAL_OK;
}
void CAN1_RX0_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan1); }
void CAN1_TX_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan1); }
void CAN1_SCE_IRQHandler(void) { HAL_CAN_IRQHandler(&hcan1); }
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) { (void)hcan; atlab_tx_ok++; }
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan) { (void)hcan; atlab_tx_ok++; }
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan) { (void)hcan; atlab_tx_ok++; }
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) { (void)hcan; ATLAB_CAN_RecordError(); }
