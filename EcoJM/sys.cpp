#include "sys.h"
#include "fd_usbxch.h"

void usb_task(void *ctx)
{
    static uint8_t fsm_state = 0;
    for(;;)
    {
        fd_usbxch_write(&Sys::usbxch, (const uint8_t*)"HELLO\n", 7);
        xTaskSleep(xMS_TO_TICKS(5000));
        xYield();
    }
}

void Sys::init()
{
    platform_init();
    fd_usbxch_init(&usbxch);
    xSchedulerInit();
    xTaskCreate(usb_task, NULL, xMS_TO_PROFTICK(1000));
    xSchedulerStart();
};