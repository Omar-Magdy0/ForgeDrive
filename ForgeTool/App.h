#pragma once
#include "ABFStream.h"
#include "Usbxch.h"
#include "TCPClient.h"
#include "Gui.h"
#include <GLFW/glfw3.h>


struct App
{
    private :
    GLFWwindow* window;
    void gui_init();
    void gui_loop();
    void gui_deinit();

    public:
    void run();
    Gui gui;
};