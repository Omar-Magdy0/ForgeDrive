#ifndef TIM2_UTIL_H
#define TIM2_UTIL_H
#include <stdint.h>
#include "fd_driver_conf.h"
#include <math.h>
#include "sil.h"

#ifdef __cplusplus
extern "C"{
#endif

void fd_hall1_init();
void fd_hall1_setComDelay_uS(uint32_t COM_delay_uS);
void fd_hall1_setComCallback(void (*callback)(void));
float fd_hall1_elec_speed();
uint8_t fd_hall1_read();

void fd_comDelay_init();
void fd_comDelay_setComDelay_uS(uint32_t COM_delay_uS);
void fd_comDelay_setComCallback(void (*callback)(void));
void sil_hall_update();

#ifdef __cplusplus
}
#endif

#endif
