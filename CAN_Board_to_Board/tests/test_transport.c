#include "main.h"
#include "atlab_debug_log.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
uint32_t mock_primask;
volatile uint8_t atlab_cdc_ready;
USBD_HandleTypeDef hUsbDeviceFS;
UART_HandleTypeDef huart2;
static USBD_CDC_HandleTypeDef cdc;
static uint8_t *in_flight;
static unsigned calls;
static uint8_t usb_result;
static HAL_StatusTypeDef uart_result;
uint8_t CDC_Transmit_FS(uint8_t *buffer, uint16_t length)
{
    assert(mock_primask && length == 5U);
    calls++;
    if (usb_result == USBD_OK) { in_flight = buffer; cdc.TxState = 1U; }
    return usb_result;
}
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *h, uint8_t *buffer, uint16_t length)
{
    assert(mock_primask && length == 5U);
    calls++;
    if (uart_result == HAL_OK) { in_flight = buffer; h->gState = 2U; }
    return uart_result;
}
int main(void)
{
    uint8_t first[] = "first", second[] = "other";
#if defined(STM32F407xx)
    assert(ATLAB_LogTransport_Send(first, 5U) == -1 && calls == 0U);
    hUsbDeviceFS.dev_state = USBD_STATE_CONFIGURED;
    assert(ATLAB_LogTransport_Send(first, 5U) == -1); /* null class */
    hUsbDeviceFS.pClassData = &cdc;
    assert(ATLAB_LogTransport_Send(first, 5U) == -1); /* DTR not asserted */
    atlab_cdc_ready = 1U;
#else
    huart2.gState = HAL_UART_STATE_READY;
#endif
    assert(ATLAB_LogTransport_Send(first, 5U) == 1 && calls == 1U);
    memset(first, 'X', 5U); /* caller may immediately reuse its own memory */
    assert(memcmp(in_flight, "first", 5U) == 0);
    for (unsigned i = 0U; i < 1000U; i++) assert(ATLAB_LogTransport_Send(second, 5U) == 0);
    assert(memcmp(in_flight, "first", 5U) == 0 && calls == 1U && mock_primask == 0U);
#if defined(STM32F407xx)
    /* Reset/deinit no longer references old bytes; next connection has idle state. */
    hUsbDeviceFS.pClassData = NULL;
    hUsbDeviceFS.dev_state = 0U;
    assert(ATLAB_LogTransport_Send(second, 5U) == -1);
    hUsbDeviceFS.pClassData = &cdc; hUsbDeviceFS.dev_state = USBD_STATE_CONFIGURED;
    cdc.TxState = 0U; usb_result = USBD_BUSY;
    assert(ATLAB_LogTransport_Send(second, 5U) == 0);
    usb_result = USBD_FAIL;
    assert(ATLAB_LogTransport_Send(second, 5U) == -1);
    usb_result = USBD_OK;
#else
    huart2.gState = HAL_UART_STATE_READY; uart_result = HAL_BUSY;
    assert(ATLAB_LogTransport_Send(second, 5U) == 0);
    uart_result = HAL_ERROR;
    assert(ATLAB_LogTransport_Send(second, 5U) == -1);
    uart_result = HAL_OK;
#endif
    mock_primask = 1U;
    assert(ATLAB_LogTransport_Send(second, 5U) == 1 && mock_primask == 1U);
    assert(memcmp(in_flight, "other", 5U) == 0);
    assert(ATLAB_LogTransport_Send(second, ATLAB_LOG_LINE_SIZE + 1U) == -1);
    puts("transport: PASS (owned async buffer, 1000 busy calls, errors, IRQ restoration; mocked HAL only)");
}
