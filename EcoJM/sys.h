#pragma once
#include "cortos.h"
#include "fd_usbxch.h"
#include "platform.h"

class Sys
{
    public:
    static void init(void);
    inline static fd_usbxch_handle_t usbxch;
};
