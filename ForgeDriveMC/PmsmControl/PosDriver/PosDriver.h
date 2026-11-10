#pragma once
#include <stdint.h>


class PosDriver
{
    static constexpr uint8_t PTICK_CTX = 1;
    static constexpr uint8_t XTICK_CTX = 2;
    uint32_t last_pTick;
    uint8_t tick_context = 0;

    public:
    void init();
    void reset(int64_t Theta_q32p31);
    void get_ThetaOmega(int64_t &Theta_q32p31, int32_t &Omega_q16p15, uint8_t pp);
    void get_eTheta(int32_t &Theta_q32p31, uint8_t pp);
    void pTick_enter(uint32_t pTick); //entering pwm loop
    void pTick_exit(); //exiting pwm loop
    void xTick_enter(uint32_t pTick); // entering motor control tick
    void xTick_exit(); // exit
    void set_commDelay(uint32_t delay_us);
};

extern PosDriver POS1;
extern PosDriver POS2;
__attribute__((weak)) void POS1_commCallback();
__attribute__((weak)) void POS2_commCallback();