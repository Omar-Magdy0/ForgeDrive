
struct SComm
{
    static constexpr float HFI_ANGLE_SCALE = 2.0f; // HFI angle spans 2 radians per cycle

    //Config runtime
    ConfigSComm conf;
    q31_t dc_vinj_q31;
    q31_t hfi_vinj_q31;

    //States
    int64_t accumulate0;
    int64_t accumulate1;
    q31_t hfi_Angv_RPT_q31;
    q31_t hfi_angle_q31;              /** High-frequency injection angle */
    volatile uint32_t eSettle_start_tick = 0;
    volatile IDStage idstage;
    volatile IDSubStage idsub;
    volatile uint16_t samples_counter; /** Remaining samples for current step. */
    
    
    //Results
    PmsmModel model;
} scomm;

void SComm_init();    /** Initialize self-commissioning. */
void SComm_onEnter(MCMode prev_mct);    /** Initialize self-commissioning. */
void SComm_onExit();
void SComm_pwmLoop(); /** Self-commissioning PWM loop. */
void SComm_xmcLoop(); /** Self-commissioning slow loop. */