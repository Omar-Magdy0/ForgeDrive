#pragma once
#include "ABFStream.h"
#include "Usbxch.h"
#include "TCPClient.h"
#include "Gui.h"

struct App
{
static constexpr uint8_t ABFRX_BUF_SIZE = 255;
static void onFrame(void *ctx, uint8_t id, uint8_t *payload, uint8_t payload_len);
static void onError(void *ctx, uint8_t id);
uint8_t abf_rx_buf[ABFRX_BUF_SIZE];
abf::Stream astream = abf::Stream(abf_rx_buf, sizeof(abf_rx_buf), this, onFrame, onError);

Gui gui;
void run();
void gui_init();
void gui_loop();
void gui_deinit();

};