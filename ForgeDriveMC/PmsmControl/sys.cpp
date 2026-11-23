#include "sys.h"
#include "fd_timer.h"
#include <stddef.h>
#include <stdalign.h>
#ifdef FD_PLATFORM_HOST
#include <thread>
#endif

extern "C" void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;

    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

void fd_mc3p_MC3P1_sync_postScanCallback(void){Sys::pmsm1.pwmLoop();}
void fd_mc3p_MC3P1_bg_postScanCallback(void) {}
void fd_timer_TIMERxMC_periodCallback(void){Sys::pmsm1.xmcLoop();}

enum class LCOM_Client : uint8_t
{
    RPC,
    DAQStream,
    DAQCodec,
    COUNT
};

enum class ABF_Channel : uint8_t
{
    RPC = 120,
    DAQStream = 121,
    DAQCodec = 127,
};




//dispatch incoming rpc command and respond
inline void RpcHandle(etl::span<uint8_t> data)
{
    size_t to_write = 0;
    alignas(max_align_t) uint8_t arg_stack[Fdmc::MAX_REQUEST_STRUCT_SIZE];
    alignas(max_align_t) uint8_t ret_stack[Fdmc::MAX_RESPONSE_STRUCT_SIZE];
    uint8_t tx_buf[Fdmc::MAX_RESPONSE_ENCODED_SIZE + 20];
    uint8_t abf_header[ABF_HEADER_SIZE];
    Rpc::DispatchStack stack = {0, 0, arg_stack, ret_stack};
    Sys::rpcServer.dispatch(data, tx_buf, stack, to_write);
    if(abf_write(abf_header, tx_buf, to_write, (uint8_t)ABF_Channel::RPC) == AbfErr_OK)
    {
        fd_usbxch_write(USB1, abf_header, ABF_HEADER_SIZE);
        fd_usbxch_write(USB1, tx_buf, to_write);
        fd_usbxch_flush(USB1);
    }
}
void FrameHandle(uint8_t id, etl::span<uint8_t> data)
{
    switch ((ABF_Channel)id)
    {
    case ABF_Channel::RPC:
        return RpcHandle(data);
    case ABF_Channel::DAQStream:
        return;
    default:
        return;
    }
}
void onPmsmFault(void *, PmsmFault err){}
void lcom(void *_ctx)
{
    TickType_t last_wake = xTaskGetTickCount();
    constexpr unsigned int LCOM_PERIOD = 1; // 1ms
    constexpr TickType_t period = pdMS_TO_TICKS(LCOM_PERIOD);
    uint8_t rx_buf[255];
    AbfReader_t frame_reader;
    AbfRead_t frame;
    frame.b_buf = rx_buf;
    frame.b_buf_sz = sizeof(rx_buf);
    abf_init_reader(&frame_reader);
    for (;;)
    {
        //Reading here 
        auto available = fd_usbxch_read_available(USB1);
        if(available)
        {
            void *r1, *r2;
            uint16_t c1, c2;
            fd_usbxch_read_reserve(USB1, &r1, &c1, &r2, &c2);
            auto process = [&](const uint8_t *data, size_t length)
            {
                size_t offset = 0;
                while (offset < length)
                {       
                    AbfReadEvent e = abf_read(&frame_reader, data + offset, length - offset, &frame);
                    offset += frame.pos;
                    if (e == AbfReadEvent_FRAME)
                    {
                        AbfHeader_t h = abf_header_decode(frame.header);
                        FrameHandle(h.id, etl::span<uint8_t>(frame.b_buf, h.size));
                    }
                    else if (e == AbfReadEvent_ERROR)
                    {

                    }
                }
            };
            if (c1)process(static_cast<const uint8_t *>(r1), c1);
            if (c2)process(static_cast<const uint8_t *>(r2), c2);
            fd_usbxch_read_commit(USB1, c1+c2);
        }
        //Check for writers requests
        //TODO: add daq stream / codec processing and writing
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
             .xtick_freq = 4000,
             .pos_rate_div = 10};

void Sys::init()
{
    fd_drv_init(&DRV); // Driver initialization
    platform_init();
    fd_usbxch_init(USB1);
    fd_timer_init(TIMERxMC, configGen.xtick_freq);
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
        PmsmErr err[8];
        uint8_t i = 0;
        ConfigPwm pwm = {
            .pwm_hz = 20000,
            .deadtime_ns = 300,
            .duty_min = 0,
            .duty_max = 0.95};
        ConfigAnalog analog = {
            .vsbus = {.offset = 1, .gain = 1},
            .vsu = {.offset = 1, .gain = 1},
            .vsv = {.offset = 1, .gain = 1},
            .vsw = {.offset = 1, .gain = 1},
            .csu_csbus = {.offset = 1, .gain = 1},
            .csv = {.offset = 1, .gain = 1},
            .csw = {.offset = 1, .gain = 1},
            .offset_calibration = true
        };
        ConfigSComm scomm = {
            .oversample_bits = 10,
            .esettle_min_ticks = 100,
            .hfi_n = 20,
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
        pmsm1.setControlMode(McMode_IDLE, MechMode_TORQUE);
        /*0*/ err[i++] = pmsm1.setConfigPwm(pwm);
        /*1*/ err[i++] = pmsm1.setConfigAnalog(analog);
        /*2*/ err[i++] = pmsm1.setConfigSComm(scomm);
        /*3*/ err[i++] = pmsm1.setPid(pid_Id, PidType_ID);
        /*4*/ err[i++] = pmsm1.setPid(pid_Iq, PidType_IQ);
        /*5*/ err[i++] = pmsm1.setPid(pid_pos, PidType_POSITION);
        /*6*/ err[i++] = pmsm1.setPid(pid_speed, PidType_SPEED);
        /*7*/ err[i++] = pmsm1.setControlMode(McMode_FOC, MechMode_TORQUE);
        /*8*/ err[i++] = pmsm1.setModel(model);
        for (int i = 0; i < 8; i++)
            if (err[i] != PmsmErr_OK)
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
    std::thread scheduler_thread([]{ vTaskStartScheduler(); });
    gui_loop();
    vTaskEndScheduler();
    scheduler_thread.join();
#endif
}



