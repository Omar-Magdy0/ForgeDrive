#include "fd_mc3p.h"
#include "platform.h"
#include "sil.h"
#include "silgui.h"
#include "fd_hall.h"


extern float vtime;
unsigned int sil_pwm_timer;
unsigned int sil_to_pwm_freq = 1;


#define FD_MC3P_TICKFREQ 1000
#define SIL_DEFAULT_VCC 36.0f
#define SIL_MAX_TIMESTEP (1.0/20000.0)
// Number of SIL steps to run for each PWM update 
//(for better resolution in the scope plots, better simulation quality and for marginally low time constants)


fd_mc3p_phase_state_t MC3P1_phase_state[3];
fd_mc3p_handle_t MC3P1;

void postScanMethod()
{
    fd_mc3p_MC3P1_sync_postScanCallback();
    sil_input.load_torque = (dummy_load.K * sil.state.omega + dummy_load.B) * sil.state.omega + dummy_load.Tc;
    for(int i = 0; i < sil_to_pwm_freq; i++) {
        sil.step(sil_input);
        sil_hall_update();
        silgui_updateData();
        silLogger.log(sil, sil_input);
    }
}


static void dPsim_dTheta_Sine(double theta_e, double dPsi_dTheta[3])
{
    double amplitude = sil.param.motor_fluxLinkage; // Back EMF amplitude
    dPsi_dTheta[0] = - amplitude * sin(theta_e);
    dPsi_dTheta[1] = - amplitude * sin(theta_e - 2*M_PI/3);
    dPsi_dTheta[2] = - amplitude * sin(theta_e + 2*M_PI/3);
};
static void dPsim_dTheta_Trapezoidal(double theta_e, double dPsi_dTheta[3])
{
    double amplitude = sil.param.motor_fluxLinkage; // Back EMF amplitude
    dPsi_dTheta[0] = - amplitude * Sil::trapezoidal_wave(theta_e);
    dPsi_dTheta[1] = - amplitude * Sil::trapezoidal_wave(theta_e - 2*M_PI/3);
    dPsi_dTheta[2] = - amplitude * Sil::trapezoidal_wave(theta_e + 2*M_PI/3);
}

void pv_mc3p_MC3P1_hw_init(fd_mc3p_handle_t *h)
{
    sil_input.dt = 1.0f / h->config.pwm_Hz / sil_to_pwm_freq;
    sil_input.load_torque = 0.00;
    sil_input.vcc = SIL_DEFAULT_VCC;
    sil.param.dPsim_dTheta = dPsim_dTheta_Sine;
    sil.param.inv_Ron = 0.008;
    sil.param.inv_Roff = 1e6;
    sil.param.motor_pp = 6;
    sil.param.motor_Rs = 0.25;
    sil.param.motor_Ls = 0.0008;
    sil.param.motor_Lm = 0.0002;
    sil.param.motor_Ms = sil.param.motor_Ls/3;
    sil.param.motor_rotorOffset = 0;
    sil.param.motor_fluxLinkage = 0.025;
    sil.param.motor_B = 1e-2;
    sil.param.motor_J = 8e-5;
    sil.param.inv_deatime_ns = h->config.deadtime_nS;
    sil.param.inv_pwm_freq = h->config.pwm_Hz;
    dummy_load.K = 1.2e-5;
    dummy_load.J = 1e-3;
    dummy_load.B = 5e-2;
    dummy_load.Tc = 0;
    sil.param.load_J = dummy_load.J;
    h->duty_max_q15 = (uint16_t)((h->config.duty_max) * 0x7FFF);
    h->duty_min_q15 = (uint16_t)((h->config.duty_min) * 0x7FFF);
    //compute deadtme compensation
    #ifdef FD_MC3P_DTC_ACTIVE
    h->dtc_comp_q15 = (int16_t)(((float)h->config.deadtime_nS*h->config.pwm_Hz/1e9)*INT16_MAX + 0.5);
    #endif
    sil_pwm_timer = register_timer(&timer_manager, postScanMethod, (uint64_t)(1e9/h->config.pwm_Hz));
}

void pv_mc3p_MC3P1_sector_update(fd_mc3p_handle_t *h, fd_mc3p_sector_t sector)
{
    
}

void pv_mc3p_MC3P1_offset_calibration(fd_mc3p_handle_t *h)
{

}


void fd_mc3p_MC3P1_reconfigure_pwm(fd_mc3p_handle_t *h)
{
    h->duty_max_q15 = (uint16_t)((h->config.duty_max) * 0x7FFF);
    h->duty_min_q15 = (uint16_t)((h->config.duty_min) * 0x7FFF);
    double min_sil_freq = 1.0/SIL_MAX_TIMESTEP;
    if(h->config.pwm_Hz < min_sil_freq)
    {
        //Find the nearest integer multiple of pwm frequency larger than minimum sil frequency
        double next_freq = 0;
        for(int i = 1; i <= 1000; i++)
        {
            if(i * h->config.pwm_Hz > min_sil_freq)
            {
                sil_to_pwm_freq = i;
            }
        }
    }
    sil_input.dt = 1.0f / h->config.pwm_Hz / sil_to_pwm_freq;
    sil.param.inv_pwm_freq = h->config.pwm_Hz;
    sil.param.inv_deatime_ns = h->config.deadtime_nS;
    configure_timer_timestep(&timer_manager, sil_pwm_timer, (uint64_t)(1e9/h->config.pwm_Hz));
}

void fd_mc3p_MC3P1_bg_startConv(fd_mc3p_handle_t *h){}

void fd_mc3p_MC3P1_read_sync(fd_mc3p_handle_t *h, void* data)
{    
    uint8_t is_svm = IS_SVM_SECTOR(h->sector_last);
    uint8_t is_trap = IS_TRAP_SECTOR(h->sector_last);
#define DTC_UPDATE_POLARITY(current, bit)         \
do                                            \
{                                             \
    if ((current) > (int32_t)FD_MC3P_DTC_CTHRESH)       \
    {                                         \
        h->dtc_state |= (bit);             \
    }                                         \
    else if ((current) < -(int32_t)FD_MC3P_DTC_CTHRESH) \
    {                                         \
        h->dtc_state &= ~(bit);            \
    }                                         \
} while (0)
    if(is_svm)
    {       
        // Convert float currents to Q31 format for the control algorithm
        // Scaling: 1.0A = X counts (adjust FD_MC3P_CS_SCALE to tune sensitivity)
        ((fd_mc3p_svm_data_t *)(data))->vbus_q31 = FD_MC3P_FLOAT_TO_VS(sil_input.vcc);
        ((fd_mc3p_svm_data_t *)(data))->cu_q31 = FD_MC3P_FLOAT_TO_CS(sil.state.ip[0]);
        ((fd_mc3p_svm_data_t *)(data))->cv_q31 = FD_MC3P_FLOAT_TO_CS(sil.state.ip[1]);
        ((fd_mc3p_svm_data_t *)(data))->cw_q31 = FD_MC3P_FLOAT_TO_CS(sil.state.ip[2]);
        #ifdef FD_MC3P_DTC_ACTIVE
        DTC_UPDATE_POLARITY(((fd_mc3p_svm_data_t *)(data))->cu_q31, (1 << 0));
        DTC_UPDATE_POLARITY(((fd_mc3p_svm_data_t *)(data))->cv_q31, (1 << 1));
        DTC_UPDATE_POLARITY(((fd_mc3p_svm_data_t *)(data))->cw_q31, (1 << 2));
        #endif
    }
    else if (is_trap)
    {
        // Back-EMF would be: Ke * ωe where ωe = pp * ω_mech
        ((fd_mc3p_svm_data_t *)(data))->vbus_q31 = FD_MC3P_FLOAT_TO_VS(sil_input.vcc);
        float adc_syncRank1, adc_syncRank2;
        switch(h->sector_last){
            case FD_MC3P_SECTOR_TRAP1:
                adc_syncRank1 = sil.state.vp[2] - sil.state.vn;
                adc_syncRank2 = sil.state.ip[0];
                break;
            case FD_MC3P_SECTOR_TRAP2:
                adc_syncRank1 = sil.state.vp[1];
                adc_syncRank2 = sil.state.ip[0];
                break;
            case FD_MC3P_SECTOR_TRAP3:
                adc_syncRank1 = sil.state.vp[0];
                adc_syncRank2 = sil.state.ip[1];
                break;
            case FD_MC3P_SECTOR_TRAP4:
                adc_syncRank1 = sil.state.vp[2];
                adc_syncRank2 = sil.state.ip[1];
                break;
            case FD_MC3P_SECTOR_TRAP5:
                adc_syncRank1 = sil.state.vp[1];
                adc_syncRank2 = sil.state.ip[2];
                break;
            case FD_MC3P_SECTOR_TRAP6:
                adc_syncRank1 = sil.state.vp[0];
                adc_syncRank2 = sil.state.ip[2];
                break;
            default:
                break;
        }
        ((fd_mc3p_trap_data_t *)(data))->vbemf_q31 = FD_MC3P_FLOAT_TO_VS(adc_syncRank1);
        ((fd_mc3p_trap_data_t *)(data))->cbus_q31  = FD_MC3P_FLOAT_TO_CS(adc_syncRank2);
        #ifdef FD_MC3P_DTC_ACTIVE
        DTC_UPDATE_POLARITY(((fd_mc3p_trap_data_t *)(data))->cbus_q31 , (1 << 3));
        #endif
    }else{
        ((fd_mc3p_svm_data_t *)(data))->vbus_q31 = FD_MC3P_FLOAT_TO_VS(sil_input.vcc);
    }
}

void fd_mc3p_MC3P1_write_phase_state(fd_mc3p_handle_t *h, fd_mc3p_phase_state_t state_u, fd_mc3p_phase_state_t state_v, fd_mc3p_phase_state_t state_w)
{
    sil_input.drive[0] = (state_u != FD_MC3P_PHASE_FLOAT)? 1 : 0;
    sil_input.drive[1] = (state_v != FD_MC3P_PHASE_FLOAT)? 1 : 0;
    sil_input.drive[2] = (state_w != FD_MC3P_PHASE_FLOAT)? 1 : 0;
    MC3P1_phase_state[0] = state_u;
    MC3P1_phase_state[1] = state_v;
    MC3P1_phase_state[2] = state_w;
}

void fd_mc3p_MC3P1_write_phase_duty(fd_mc3p_handle_t *h, int16_t duty_u_q15, int16_t duty_v_q15, int16_t duty_w_q15)
{
    duty_u_q15 = (uint32_t)SATURATE(duty_u_q15, h->duty_min_q15, h->duty_max_q15);
    duty_v_q15 = (uint32_t)SATURATE(duty_v_q15, h->duty_min_q15, h->duty_max_q15);
    duty_w_q15 = (uint32_t)SATURATE(duty_w_q15, h->duty_min_q15, h->duty_max_q15);
    float duty[3] = {((float)duty_u_q15/INT16_MAX), ((float)duty_v_q15/INT16_MAX), ((float)duty_w_q15/INT16_MAX)};
    for(int i = 0; i < 3; i++)
    {
        if(MC3P1_phase_state[i] == FD_MC3P_PHASE_L_ON)
        {
            duty[i] = 0;
        }else if (MC3P1_phase_state[i] == FD_MC3P_PHASE_H_ON)
        {
            duty[i] = 1;
        }
        sil_input.duty[i] = duty[i];
    }
}
