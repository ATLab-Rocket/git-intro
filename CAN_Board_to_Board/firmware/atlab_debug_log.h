#ifndef ATLAB_DEBUG_LOG_H
#define ATLAB_DEBUG_LOG_H
#include <stdint.h>
#define ATLAB_LOG_LINE_SIZE 192U
#define ATLAB_LOG_QUEUE_SIZE 16U
extern volatile uint32_t atlab_log_calls, atlab_log_drops, atlab_log_busy, atlab_log_offline;
extern char atlab_last_log[ATLAB_LOG_LINE_SIZE];
/* Main-loop only; never format logs in an ISR. */
void ATLAB_Log(const char *format, ...);
void ATLAB_Log_Service(void);
/* >0 accepted with owned copy; 0 busy/retry; <0 offline/drop. Never waits. */
int ATLAB_LogTransport_Send(const uint8_t *data, uint16_t length);
#endif
