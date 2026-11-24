#include "atlab_debug_log.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if defined(ATLAB_USE_USB_CDC)
#include "usbd_cdc_if.h"
#endif

volatile uint32_t atlab_log_calls = 0U;
volatile uint32_t atlab_log_drops = 0U;
char atlab_last_log[128];

void ATLAB_Log(const char *format, ...)
{
    va_list args;
    int len;

    va_start(args, format);
    len = vsnprintf(atlab_last_log, sizeof(atlab_last_log), format, args);
    va_end(args);
    atlab_log_calls++;

    if (len < 0) { atlab_log_drops++; return; }

#if defined(ATLAB_USE_USB_CDC)
    /* Called only from the main loop; the HAL CDC implementation
     * may return BUSY and this demo intentionally drops that line.
     */
    uint16_t used = (uint16_t)strlen(atlab_last_log);
    if (CDC_Transmit_FS((uint8_t *)atlab_last_log, used) != USBD_OK)
        atlab_log_drops++;
#else
    /* Keep the most recent log in RAM for a debugger. */
    (void)len;
#endif
}
