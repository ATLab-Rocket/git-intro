#include "main.h"
#include "atlab_debug_log.h"
#include <string.h>
#if defined(STM32F407xx)
#include "usb_device.h"
#include "usbd_cdc_if.h"
extern USBD_HandleTypeDef hUsbDeviceFS;
extern volatile uint8_t atlab_cdc_ready;
#else
extern UART_HandleTypeDef huart2;
#endif
/* IRQs reference these bytes after the call returns. Never overwrite while busy. */
static uint8_t tx_buffer[ATLAB_LOG_LINE_SIZE];
int ATLAB_LogTransport_Send(const uint8_t *data, uint16_t length)
{
    int result;
    if (length > sizeof(tx_buffer)) return -1;
    uint32_t irq = __get_PRIMASK();
    __disable_irq();
#if defined(STM32F407xx)
    /* Atomic with reset/disconnect. Never cache class data across calls. */
    USBD_CDC_HandleTypeDef *cdc = (USBD_CDC_HandleTypeDef *)hUsbDeviceFS.pClassData;
    if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED || cdc == NULL || !atlab_cdc_ready) {
        result = -1;
    } else if (cdc->TxState != 0U) {
        result = 0;
    } else {
        memcpy(tx_buffer, data, length);
        uint8_t status = CDC_Transmit_FS(tx_buffer, length);
        result = status == USBD_OK ? 1 : (status == USBD_BUSY ? 0 : -1);
    }
#else
    if (huart2.gState != HAL_UART_STATE_READY) {
        result = 0;
    } else {
        memcpy(tx_buffer, data, length);
        HAL_StatusTypeDef status = HAL_UART_Transmit_IT(&huart2, tx_buffer, length);
        result = status == HAL_OK ? 1 : (status == HAL_BUSY ? 0 : -1);
    }
#endif
    __set_PRIMASK(irq);
    return result;
}
