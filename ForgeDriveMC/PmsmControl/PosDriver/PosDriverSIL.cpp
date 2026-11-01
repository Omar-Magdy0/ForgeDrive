#include "PosDriver.h"
#include "sil.h"
#include <math.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

PosDriver POS1;
__attribute__((weak)) void POS1_commCallback(){};

void PosDriver::init()
{

}
void PosDriver::reset(int64_t Theta_q32p31){}
void PosDriver::get_ThetaOmega(int64_t &Theta_q32p31, int32_t &Omega_q16p15, uint8_t pp)
{
    Theta_q32p31 = sil.state.theta*(INT32_MAX / M_PI);
    Omega_q16p15 = sil.state.omega*(INT16_MAX / M_PI);
}
void PosDriver::get_eTheta(int32_t &Theta_q31, uint8_t pp)
{
    double theta_e = std::remainder(
        sil.state.theta * sil.param.motor_pp,
        2.0 * M_PI
    );

    Theta_q31 = static_cast<int32_t>(
        theta_e * (INT32_MAX / M_PI)
    );
}
void PosDriver::pTick_enter(uint32_t pTick)
{
    //Operations here
    last_pTick = pTick;
    tick_context = PTICK_CTX;
};
void PosDriver::pTick_exit(){}
void PosDriver::xTick_enter(uint32_t pTick)
{
    last_pTick = pTick;
    tick_context = XTICK_CTX;
}
void PosDriver::xTick_exit(){}
void PosDriver::set_commDelay(uint32_t delay_us){}

