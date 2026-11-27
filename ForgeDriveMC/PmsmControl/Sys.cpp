#include "Sys.h"
#include "Abf.h"
#include "Timer.h"
#include "fdmc.pb.h"
#include <stdalign.h>
#include <stddef.h>
#ifdef FDDRV_PLATFORM_HOST
#include <thread>
#endif
extern "C" void vApplicationStackOverflowHook(TaskHandle_t task, char* name) {
  (void)task;
  (void)name;
  taskDISABLE_INTERRUPTS();
  for (;;) {}
}
void FdMc3p_MC3P1_SyncPostScanCallback(void) { Sys::pmsm1.PwmLoop(); }
void FdMc3p_MC3P1_BgPostScanCallback(void) {}
void FdTimer_TIMERxMC_PeriodCallback(void) { Sys::pmsm1.XmcLoop(); }
// dispatch incoming rpc command and respond
inline void RpcHandle(etl::span<uint8_t> data) {
  size_t                       toWrite = 0;
  alignas(max_align_t) uint8_t argStack[Fdmc::MaxArgStructSz];
  alignas(max_align_t) uint8_t retStack[Fdmc::MaxRetStructSz];
  uint8_t                      txBuf[Fdmc::MaxRetEncodeSz + 20];
  uint8_t                      abfHdr[ABF_HDR_SZ];
  Rpc::DispatchStack           stack = {0, 0, argStack, retStack};
  Sys::rpcServer.Dispatch(data, txBuf, stack, toWrite);
  if (Abf_Write(abfHdr, txBuf, toWrite, (uint8_t)AbfId_Rpc) == AbfErr_Ok) {
    FdUsbxch_Write(USB1, abfHdr, ABF_HDR_SZ);
    FdUsbxch_Write(USB1, txBuf, toWrite);
    FdUsbxch_Flush(USB1);
  }
}
void FrameHandle(uint8_t id, etl::span<uint8_t> data) {
  switch (id) {
  case AbfId_Rpc: return RpcHandle(data);
  case AbfId_DaqStream: return;
  default: return;
  }
}
void onPmsmFault(void*, PmsmFault err) {}
void lcom(void* _ctx) {
  TickType_t             last_wake   = xTaskGetTickCount();
  constexpr unsigned int LCOM_PERIOD = 1; // 1ms
  constexpr TickType_t   period      = pdMS_TO_TICKS(LCOM_PERIOD);
  uint8_t                rxBuf[255];
  AbfReader              frameReader;
  AbfRead                frame;
  frame.bBuf   = rxBuf;
  frame.bBufSz = sizeof(rxBuf);
  Abf_InitReader(&frameReader);
  for (;;) {
    // Reading here
    auto available = FdUsbxch_ReadAvailable(USB1);
    if (available) {
      void *   r1, *r2;
      uint16_t c1, c2;
      FdUsbxch_ReadReserve(USB1, &r1, &c1, &r2, &c2);
      auto Process = [&](const uint8_t* data, size_t length) {
        size_t offset = 0;
        while (offset < length) {
          AbfReadEvent e  = Abf_Read(&frameReader, data + offset, length - offset, &frame);
          offset         += frame.pos;
          if (e == AbfReadEvent_Frame) {
            AbfHdr h = Abf_HdrDecode(frame.hdr);
            FrameHandle(h.id, etl::span<uint8_t>(frame.bBuf, h.sz));
          } else if (e == AbfReadEvent_Err) {
          }
        }
      };
      if (c1) Process(static_cast<const uint8_t*>(r1), c1);
      if (c2) Process(static_cast<const uint8_t*>(r2), c2);
      FdUsbxch_ReadCommit(USB1, c1 + c2);
    }
    // Check for writers requests
    // TODO: add daq stream / codec processing and writing
    vTaskDelayUntil(&last_wake, period);
  }
}
ConfigGen Sys::configGen = {.maxPwm_hz      = 40000,
                            .minPwm_hz      = 5000,
                            .maxDeadtime_ns = 1000,
                            .minDeadtime_ns = 300,
                            .maxCMag        = 20,
                            .minVBus        = 30,
                            .maxVBus        = 40,
                            .xtick_hz       = 4000,
                            .posRate        = 10};
void      Sys::init() {
  FdDrv_Init(&DRV); // Driver initialization
  platform_init();
  FdUsbxch_Init(USB1);
  FdTimer_Init(TIMERxMC, configGen.xtick_hz);
  FdTimer_Enable(TIMERxMC, true);
  lcom_handle    = xTaskCreateStatic(lcom, "lcom", LCOM_STACK_SIZE, NULL, LCOM_PRIO, lcom_stack, &lcom_buf);
  bool config_ok = true;
  // enable motor control
  {
    PmsmErr      err[8];
    uint8_t      i        = 0;
    ConfigPwm    pwm      = {.pwm_hz = 20000, .deadtime_ns = 600, .minDuty = 0, .maxDuty = 0.95, .dtcThresh = 0, .dtcActive = false};
    ConfigAnalog analog   = {.vsBus       = {.offset = 1, .gain = 1},
                             .vsU         = {.offset = 1, .gain = 1},
                             .vsV         = {.offset = 1, .gain = 1},
                             .vsW         = {.offset = 1, .gain = 1},
                             .csU_csBus   = {.offset = 1, .gain = 1},
                             .csV         = {.offset = 1, .gain = 1},
                             .csW         = {.offset = 1, .gain = 1},
                             .offsetCalib = true};
    ConfigScomm  scomm    = {.oversampleBits = 10, .minSettleTicks = 100, .hfiN = 20, .dcVinj = 2.5, .hfiVinj = 2};
    PmsmModel    model    = {.res_ohm = 0.1, .indD_h = 0.001, .indQ_h = 0.001, .flux_wb = 1, .inertia_kgm2 = 0.1, .friction_nms = 0.1, .polePairs = 1};
    Pid          pidCD    = {.kp = 0.1, .ki = 0.01, .kd = 0, .type = PidType_CD};
    Pid          pidCQ    = {.kp = 0.1, .ki = 0.01, .kd = 0, .type = PidType_CQ};
    Pid          pidPos   = {.kp = 0.1, .ki = 1e-5, .kd = 1e-5, .type = PidType_Position};
    Pid          pidSpeed = {.kp = 0.1, .ki = 0.5, .kd = 0, .type = PidType_Speed};
    ControlMode  ctrl     = {McMode_Idle, MechMode_Torque};
    pmsm1.SetControlMode(ctrl);
    /*0*/ err[i++] = pmsm1.SetConfigPwm(pwm);
    /*1*/ err[i++] = pmsm1.SetConfigAnalog(analog);
    /*2*/ err[i++] = pmsm1.SetConfigScomm(scomm);
    /*3*/ err[i++] = pmsm1.SetModel(model);
    /*4*/ err[i++] = pmsm1.SetPid(pidCD);
    /*5*/ err[i++] = pmsm1.SetPid(pidCQ);
    /*6*/ err[i++] = pmsm1.SetPid(pidPos);
    /*7*/ err[i++] = pmsm1.SetPid(pidSpeed);
    ctrl.mcMode    = McMode_Foc;
    /*8*/ err[i++] = pmsm1.SetControlMode(ctrl);
    for (int i = 0; i < 8; i++)
      if (err[i] != PmsmErr_Ok) config_ok = false;
  }
  while (!config_ok);
  pmsm1.Init();
  pmsm1.SetSetpoint({5});
// FreeRtos initialization
#ifndef FDDRV_PLATFORM_HOST
  vTaskStartScheduler();
  while (true); // never reach here
#else
  std::thread scheduler_thread([] { vTaskStartScheduler(); });
  gui_loop();
  vTaskEndScheduler();
  scheduler_thread.join();
#endif
}
