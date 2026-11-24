#include "atlab_debug_log.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int result;
static char captured[ATLAB_LOG_LINE_SIZE];
int ATLAB_LogTransport_Send(const uint8_t *data, uint16_t length)
{
    if (result > 0) { memcpy(captured, data, length); captured[length] = 0; }
    return result;
}
int main(void)
{
    ATLAB_Log("first %u", 7U);
    result = 0;
    ATLAB_Log_Service();
    ATLAB_Log("second");
    result = 1;
    ATLAB_Log_Service();
    assert(strcmp(captured, "first 7") == 0 && atlab_log_busy == 1U);
    ATLAB_Log_Service();
    assert(strcmp(captured, "second") == 0);
    for (unsigned i = 0; i < ATLAB_LOG_QUEUE_SIZE + 3U; i++) ATLAB_Log("%u", i);
    assert(atlab_log_drops == 3U);
    for (unsigned i = 0; i < ATLAB_LOG_QUEUE_SIZE; i++) {
        ATLAB_Log_Service();
        char expected[8]; (void)snprintf(expected, sizeof(expected), "%u", i);
        assert(strcmp(captured, expected) == 0);
    }
    char big[ATLAB_LOG_LINE_SIZE + 1U]; memset(big, 'x', sizeof(big)); big[sizeof(big)-1U] = 0;
    ATLAB_Log("%s", big);
    assert(atlab_log_drops == 4U); /* reject truncation */
    result = -1;
    ATLAB_Log("offline"); ATLAB_Log_Service();
    assert(atlab_log_offline == 1U && atlab_log_drops == 5U);
    result = 1;
    ATLAB_Log("reconnected"); ATLAB_Log_Service();
    assert(strcmp(captured, "reconnected") == 0);
    puts("logging queue: PASS (busy/retry, FIFO, overflow, truncation, offline/reconnect)");
}
