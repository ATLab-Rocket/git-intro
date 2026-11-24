#include "atlab_debug_log.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
volatile uint32_t atlab_log_calls, atlab_log_drops, atlab_log_busy, atlab_log_offline;
char atlab_last_log[ATLAB_LOG_LINE_SIZE];
static char queue[ATLAB_LOG_QUEUE_SIZE][ATLAB_LOG_LINE_SIZE];
static uint16_t lengths[ATLAB_LOG_QUEUE_SIZE];
static unsigned head, tail, count;
void ATLAB_Log(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int n = vsnprintf(atlab_last_log, sizeof(atlab_last_log), format, args);
    va_end(args);
    atlab_log_calls++;
    if (n < 0 || (unsigned)n >= sizeof(atlab_last_log) || count == ATLAB_LOG_QUEUE_SIZE) {
        atlab_log_drops++;
        return;
    }
    memcpy(queue[tail], atlab_last_log, (size_t)n + 1U);
    lengths[tail] = (uint16_t)n;
    tail = (tail + 1U) % ATLAB_LOG_QUEUE_SIZE;
    count++;
}
void ATLAB_Log_Service(void)
{
    if (count == 0U) return;
    /* Transport copies to its OWN persistent buffer before returning success. */
    int result = ATLAB_LogTransport_Send((const uint8_t *)queue[head], lengths[head]);
    if (result > 0) {
        head = (head + 1U) % ATLAB_LOG_QUEUE_SIZE;
        count--;
    } else if (result == 0) {
        atlab_log_busy++;
    } else {
        /* Drop stale offline records; CAN continues without waiting for USB. */
        atlab_log_offline++;
        atlab_log_drops++;
        head = (head + 1U) % ATLAB_LOG_QUEUE_SIZE;
        count--;
    }
}
