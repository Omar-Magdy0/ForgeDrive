#include "sys.h"


void Sys::init()
{
    xSchedulerInit();
    xSchedulerStart();
    xTaskNotifyWait(110);
};