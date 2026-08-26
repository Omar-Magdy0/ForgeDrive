#include "usbd_conf.h"
// USB interrupt defined eslewhere due to
extern PCD_HandleTypeDef hpcd;

void USB_LP_CAN1_RX0_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd);
}