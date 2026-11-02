#include "PosDriver.h"


void PosDriver::init(){}
void PosDriver::reset(int64_t Theta_q32p31){}
void PosDriver::get_ThetaOmega(int64_t &Theta_q32p31, int32_t &Omega_q16p15, uint8_t pp){}
void PosDriver::get_eTheta(int32_t &Theta_q32p31, uint8_t pp){}
void PosDriver::pTick_enter(uint32_t pTick){} //entering pwm loop
void PosDriver::pTick_exit(){} //exiting pwm loop
void PosDriver::xTick_enter(uint32_t pTick){} // entering motor control tick
void PosDriver::xTick_exit(){} // exit
void PosDriver::set_commDelay(uint32_t delay_us){}

PosDriver POS1;