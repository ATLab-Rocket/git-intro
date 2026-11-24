#ifndef MOCK_USB_DEVICE_H
#define MOCK_USB_DEVICE_H
#include <stdint.h>
typedef struct { void *pClassData; uint32_t dev_state; } USBD_HandleTypeDef;
#define USBD_STATE_CONFIGURED 3U
#endif
