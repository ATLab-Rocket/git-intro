#ifndef ATLAB_DEMO_H
#define ATLAB_DEMO_H
#include "main.h"
#include "atlab_session.h"
void ATLAB_Demo_Run(void);
extern ATLAB_Session atlab_session;
extern volatile uint32_t atlab_rx_ok, atlab_bad_frame;
#endif
