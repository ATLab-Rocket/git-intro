#ifndef MOCK_CDC_H
#define MOCK_CDC_H
#include <stdint.h>
typedef struct { volatile uint32_t TxState; } USBD_CDC_HandleTypeDef;
#define USBD_OK 0U
#define USBD_BUSY 1U
#define USBD_FAIL 2U
uint8_t CDC_Transmit_FS(uint8_t *, uint16_t);
#endif
