
struct Scomm {
  static constexpr float HFI_ANGLE_SCALE = 2.0f; // HFI angle spans 2 radians per cycle
  // Config runtime
  ConfigScomm conf;
  int32_t     dcVinj_Q31;
  int32_t     hfiVinj_Q31;
  // States
  int64_t             accumulate0;
  int64_t             accumulate1;
  int32_t             hfiOmegaRpt;  // Rad per tick
  int32_t             hfiTheta_Q31; /** High-frequency injection angle */
  volatile uint32_t   eSettleStartTick = 0;
  volatile IdStage    idStage;
  volatile IdSubStage idSub;
  volatile uint16_t   sampleCnt; /** Remaining samples for current step. */
  // Results
  PmsmModel model;
} scomm;
void SComm_init();                   /** Initialize self-commissioning. */
void SComm_onEnter(McMode prev_mct); /** Initialize self-commissioning. */
void SComm_onExit();
void SComm_pwmLoop(); /** Self-commissioning PWM loop. */
void SComm_xmcLoop(); /** Self-commissioning slow loop. */