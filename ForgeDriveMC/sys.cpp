#include "sys.h"
#include "fd_timer.h"
#include "RPC.hpp"
#ifdef FD_PLATFORM_HOST
#include <thread>
#endif

using namespace ForgeDriveMC_RPC;
extern "C" void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;

    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

void fd_mc3p_MC3P1_sync_postScanCallback(void)
{
    Sys::pmsm1.pwmLoop();
}
void fd_mc3p_MC3P1_bg_postScanCallback(void) {}
void fd_timer_TIMERxMC_periodCallback(void)
{
    Sys::pmsm1.xmcLoop();
}


enum class LCOM_Client : uint8_t
{
    RPC,
    DAQStream,
    DAQCodec,
    COUNT
};

enum ABF_Channel : uint8_t
{
    RPC = 120,
    DAQStream = 121,
    DAQCodec = 127,
};


void Sys::onFrame(void *ctx, uint8_t id, uint8_t *payload, uint8_t payload_len)
{
    switch ((ABF_Channel)id)
    {
    case ABF_Channel::RPC:
    {
        //dispatch incoming rpc command and respond
        TCborParse parse(etl::span<const uint8_t>(payload, payload_len));
        TCborEncode encode(etl::span<uint8_t>(tx_buf, TX_BUF_SIZE));
        RPC_DISPATCH(parse, encode);
        auto size = encode.size();
        abfStream.encode(abf_header_buf, tx_buf, ABF_Channel::RPC, size);
        fd_usbxch_write(USB1, abf_header_buf, abf::HEADER_SIZE);
        fd_usbxch_write(USB1, tx_buf, size);
        fd_usbxch_flush(USB1);
        break;
    }
    case ABF_Channel::DAQStream:
    {

        break;
    }
    default:
        break;
    }

}
void Sys::onError(void *ctx, uint8_t id)
{
}
void Sys::onPmsmFault(void *ctx, PMSM_FAULT f)
{
}

void lcom(void *arg)
{
    TickType_t last_wake = xTaskGetTickCount();
    constexpr unsigned int LCOM_PERIOD = 1; // 1ms
    constexpr TickType_t period = pdMS_TO_TICKS(LCOM_PERIOD);
    for (;;)
    {
        //Reading here 
        auto available = fd_usbxch_read_available(USB1);
        if(available)
        {
            void *r1, *r2;
            uint16_t c1, c2;
            fd_usbxch_read_reserve(USB1, &r1, &c1, &r2, &c2);
            if(c1)Sys::abfStream.process(static_cast<const uint8_t*>(r1), c1);
            if(c2)Sys::abfStream.process(static_cast<const uint8_t*>(r2), c2);
            fd_usbxch_read_commit(USB1, c1+c2);
        }
        //Check for writers requests

        vTaskDelayUntil(&last_wake, period); 
    }
}

ConfigGen Sys::configGen =             
            {.max_freq = 40000,
             .min_freq = 5000,
             .max_deadtime_ns = 1000,
             .min_deadtime_ns = 200,
             .max_cmag = 20,
             .min_vbus = 30,
             .max_vbus = 40,
             .xTick_freq = 4000,
             .pos_rate_div = 10};

void Sys::init()
{
    fd_drv_init(&DRV); // Driver initialization
    platform_init();
    fd_usbxch_init(USB1);
    fd_timer_init(TIMERxMC, configGen.xTick_freq);
    fd_timer_enable(TIMERxMC, true);
    lcom_handle = xTaskCreateStatic(lcom,
                                    "lcom",
                                    LCOM_STACK_SIZE,
                                    NULL,
                                    LCOM_PRIO,
                                    lcom_stack,
                                    &lcom_buf);
    bool config_ok = true;
    // enable motor control
    {
        PMSM_ERR err[8];
        uint8_t i = 0;
        ConfigPwm pwm = {
            .pwm_Hz = 20000,
            .deadtime_ns = 300,
            .duty_min = 0,
            .duty_max = 0.95};
        ConfigAnalog analog = {
            .sync_scale = {
                {1, 1},
                {1, 1},
                {1, 1},
                {1, 1},
                {1, 1},
                {1, 1},
                {1, 1}},
            .offset_calibration = true};
        ConfigSComm scomm = {
            .oversample_bits = 10,
            .esettle_min_ticks = 100,
            .hfi_N = 20,
            .dc_vinj = 2.5,
            .hfi_vinj = 2};
        PmsmModel model = {
            .R = 0.1,
            .Ld = 0.001,
            .Lq = 0.001,
            .psi = 1,
            .J = 0.1,
            .B = 0.1,
            .pole_pairs = 1};
        Pid pid_Id, pid_Iq, pid_pos, pid_speed;
        pid_Id = {.Kp = 0.1, .Ki = 0.01, .Kd = 0};
        pid_Iq = {.Kp = 0.1, .Ki = 0.01, .Kd = 0};
        pid_pos = {.Kp = 0.1, .Ki = 1e-5, .Kd = 1e-5};
        pid_speed = {.Kp = 0.1, .Ki = 0.5, .Kd = 0};
        pmsm1.setControlMode(MCMode::Idle, MechMode::Torque);
        /*0*/ err[i++] = pmsm1.setConfigPwm(pwm);
        /*1*/ err[i++] = pmsm1.setConfigAnalog(analog);
        /*2*/ err[i++] = pmsm1.setConfigSComm(scomm);
        /*3*/ err[i++] = pmsm1.setPid(pid_Id, PidType::Id);
        /*4*/ err[i++] = pmsm1.setPid(pid_Iq, PidType::Iq);
        /*5*/ err[i++] = pmsm1.setPid(pid_pos, PidType::Position);
        /*6*/ err[i++] = pmsm1.setPid(pid_speed, PidType::Speed);
        /*7*/ err[i++] = pmsm1.setControlMode(MCMode::Foc, MechMode::Torque);
        /*8*/ err[i++] = pmsm1.setModel(model);
        for (int i = 0; i < 8; i++)
            if (err[i] != PMSM_ERR::OK)
                config_ok = false;
    }
    while (!config_ok)
        ;
    pmsm1.init();
    pmsm1.setSetpoint(5);
// FreeRtos initialization
#ifndef FD_PLATFORM_HOST
    vTaskStartScheduler();
    while (true)
        ; // never reach here
#else
    std::thread scheduler_thread([]
                                 { vTaskStartScheduler(); });
    while (true)
    {
        gui_loop();
    }
    vTaskEndScheduler();
    scheduler_thread.join();
#endif
}
