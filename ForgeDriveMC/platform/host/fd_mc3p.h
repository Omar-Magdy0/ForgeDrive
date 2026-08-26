#ifndef MCADCPWM3P_H
#define MCADCPWM3P_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MC3P_SYNC_CHANNELS 7
#include <stdbool.h>
typedef enum {
    FD_MC3P_SECTOR_FLOAT = 0,
    FD_MC3P_SECTOR_TRAP1,
    FD_MC3P_SECTOR_TRAP2,
    FD_MC3P_SECTOR_TRAP3,
    FD_MC3P_SECTOR_TRAP4,
    FD_MC3P_SECTOR_TRAP5,
    FD_MC3P_SECTOR_TRAP6,
    FD_MC3P_SECTOR_SVM1,
    FD_MC3P_SECTOR_SVM2,
    FD_MC3P_SECTOR_SVM3,
    FD_MC3P_SECTOR_SVM4,
    FD_MC3P_SECTOR_SVM5,
    FD_MC3P_SECTOR_SVM6
} fd_mc3p_sector_t;

typedef enum {
    FD_MC3P_MODE_NONE = 0,
    FD_MC3P_MODE_TRAP,
    FD_MC3P_MODE_SVM 
} fd_mc3p_mode_t;

#define IS_SVM_SECTOR(sector) (sector >= FD_MC3P_SECTOR_SVM1 && sector <= FD_MC3P_SECTOR_SVM6)
#define IS_TRAP_SECTOR(sector) (sector >= FD_MC3P_SECTOR_TRAP1 && sector <= FD_MC3P_SECTOR_TRAP6)

typedef enum {
    FD_MC3P_PHASE_FLOAT = 0,
    FD_MC3P_PHASE_H_PWM,
    FD_MC3P_PHASE_L_PWM,
    FD_MC3P_PHASE_COMP,
    FD_MC3P_PHASE_H_ON,
    FD_MC3P_PHASE_L_ON
} fd_mc3p_phase_state_t;  

typedef enum{
    FD_MC3P_VSBUS = 0,
    FD_MC3P_VSU,
    FD_MC3P_VSV,
    FD_MC3P_VSW,
    FD_MC3P_CSU,
    FD_MC3P_CSV,
    FD_MC3P_CSW,
    FD_MC3P_CSBUS = FD_MC3P_CSU,
}fd_mc3p_sync;

typedef struct{
    uint32_t pwm_Hz;
    uint32_t deadtime_nS;
    float duty_max;
    float duty_min;
}fd_mc3p_config_t;



typedef struct{
    fd_mc3p_config_t config;
    int16_t adc_to_uV;
    float adc_ref_V;
    float internal_ref_V;
    volatile fd_mc3p_mode_t mode;
    volatile fd_mc3p_sector_t sector_last;
    uint32_t timer_max_q15;
    uint16_t duty_max_q15;
    uint16_t duty_min_q15;
    int16_t dutyu_q15;
    int16_t dutyv_q15;
    int16_t dutyw_q15;
    int32_t sync_scale_q31[MC3P_SYNC_CHANNELS][2];
    uint8_t sync_rank_scale[4];
    bool offset_calibration;
    int phase_state[3];
    uint16_t dtc_comp_q15;
    uint8_t dtc_state;
}fd_mc3p_handle_t;

typedef struct
{   
    int32_t vbus_q31;
    int32_t cu_q31;
    int32_t cv_q31;
    int32_t cw_q31;
} fd_mc3p_svm_data_t;

typedef struct{
    int32_t vbus_q31;
    int32_t vbemf_q31;
    int32_t cbus_q31;
} fd_mc3p_trap_data_t;

#define FD_MC3P_VS_TO_FLOAT(vs)(((float)vs*FD_MC3P_VS_SCALE) / INT32_MAX )
#define FD_MC3P_CS_TO_FLOAT(cs)(((float)cs*FD_MC3P_CS_SCALE) / INT32_MAX )
#define FD_MC3P_FLOAT_TO_VS(f)((int32_t)(((f)/FD_MC3P_VS_SCALE) * INT32_MAX ))   
#define FD_MC3P_FLOAT_TO_CS(f)((int32_t)(((f)/FD_MC3P_CS_SCALE) * INT32_MAX ))


//TODO  FINISH ADC IMPLEMENTATION FOR 1)TRAP & 2)SVM
void fd_mc3p_init(fd_mc3p_handle_t *h);
void fd_mc3p_reconfigure_pwm(fd_mc3p_handle_t *h);
void fd_mc3p_set_gain(fd_mc3p_handle_t *h, fd_mc3p_sync s, float gain);
void fd_mc3p_set_sync_scale(fd_mc3p_handle_t *h, const float scales[MC3P_SYNC_CHANNELS][2]);
void fd_mc3p_bg_startConv(fd_mc3p_handle_t *h);
uint8_t fd_mc3p_bg_channels(fd_mc3p_handle_t *h);
float fd_mc3p_adc_read_single(fd_mc3p_handle_t *h, uint32_t channel);

uint8_t fd_mc3p_read_bg(fd_mc3p_handle_t *h, float *scanData);
uint8_t fd_mc3p_bg_isReady(fd_mc3p_handle_t *h);
void fd_mc3p_read_sync(fd_mc3p_handle_t *h, void* scanData);

void fd_mc3p_write_phase_state(fd_mc3p_handle_t *h, fd_mc3p_phase_state_t state_u, fd_mc3p_phase_state_t state_v, fd_mc3p_phase_state_t state_w);
void fd_mc3p_write_phase_duty(fd_mc3p_handle_t *h, int16_t duty_u_q15, int16_t duty_v_q15, int16_t duty_w_q15);

void fd_mc3p_write_float(fd_mc3p_handle_t *h);
void fd_mc3p_write_trap(fd_mc3p_handle_t *h, fd_mc3p_sector_t sector, uint16_t duty_q15);
void fd_mc3p_write_svm(fd_mc3p_handle_t *h, int16_t alpha_q15, int16_t beta_q15);

__attribute__((weak)) void fd_xmc3p_tickerCallback(void);
__attribute__((weak)) void fd_mc3p_sync_postScanCallback(void);

#ifdef __cplusplus
}
#endif


#endif


