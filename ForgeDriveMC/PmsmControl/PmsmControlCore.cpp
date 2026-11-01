#include "PmsmControlCore.h"

using namespace PmsmControlTypes;

ERR PmsmControlCore::init()
{
    pTicks = 0;
    xTicks = 0;
    modelUpdate();
    configGenUpdate();
    /* mode initialization*/
    voltage_q31_to_float(state.Vbus_q31);
    current_q31_to_float(state.Ibus_q31);
    angle_q31_to_float(state.eTheta_q31);
    SComm_init();
    Foc_init();
    /* hardware initialization */
    POS1.init();
    fd_mc3p_init(MC3P1);
    fd_mc3p_write_float(MC3P1);
    return ERR::OK;
};
void PmsmControlCore::configGenUpdate()
{
    control.max_cmag_q31 = FD_MC3P_FLOAT_TO_CS(CLAMP(ABS(conf.max_cmag), 0, FD_MC3P_CS_SCALE));
    control.min_vbus_q31 = FD_MC3P_FLOAT_TO_VS(CLAMP(ABS(conf.min_vbus), 0, FD_MC3P_VS_SCALE));
    control.max_vbus_q31 = FD_MC3P_FLOAT_TO_VS(CLAMP(ABS(conf.max_vbus), 0, FD_MC3P_VS_SCALE));
    xTicks_period_ns = 1'000'000'000 / conf.xTick_freq;
}
void PmsmControlCore::modelUpdate()
{
    control.inv_kt = 1 / (model.psi * model.pole_pairs);
}
void PmsmControlCore::xmcLoop()
{
    if (control.mc_mode == MCMode::None)
        return;
    switch (control.mc_mode)
    {
    case MCMode::Foc:
        Foc_xmcLoop();
        break;
    case MCMode::SComm:
        SComm_xmcLoop();
        break;
    default:
        break;
    }
    xTicks = xTicks + 1;
}
void PmsmControlCore::pwmConfigUpdate(PmsmControlTypes::ConfigPwm cfg)
{
    pwm_freq_hz = cfg.pwm_Hz;
    pTick_period_ns = static_cast<uint32_t>(1'000'000'000 / static_cast<float>(pwm_freq_hz));
    MC3P1.config.pwm_Hz = pwm_freq_hz;
    MC3P1.config.deadtime_nS = cfg.deadtime_ns;
    MC3P1.config.duty_min = cfg.duty_min;
    MC3P1.config.duty_max = cfg.duty_max;
    fd_mc3p_reconfigure_pwm(MC3P1);
}
void PmsmControlCore::pwmLoop()
{
    if (control.mc_mode == MCMode::None)
        return;
    uint32_t start = fd_dwt_tick();
    fd_mc3p_read_sync(MC3P1, &mc3p_sync_meas);
    switch (control.mc_mode)
    {
    case MCMode::Foc:
        Foc_pwmLoop();
        break;
    case MCMode::SComm:
        SComm_pwmLoop();
        break;
    default:
        break;
    }
    pTicks = pTicks + 1;
    volatile uint32_t elapsed = fd_dwt_tick() - start;
}
void PmsmControlCore::setControlMode(MCMode mc_mode)
{
    if (control.mc_mode != mc_mode)
    {
        // Exit current mode
        switch (control.mc_mode)
        {
        case MCMode::Foc:
            Foc_onExit();
            break;
        case MCMode::SComm:
            SComm_onExit();
            break;
        default:
            break;
        }
        // Enter new mode
        switch (mc_mode)
        {
        case MCMode::Foc:
            Foc_onEnter(control.mc_mode);
            break;
        case MCMode::SComm:
            SComm_onEnter(control.mc_mode);
            break;
        default:
            break;
        }
        control.mc_mode = mc_mode;
    }
}
