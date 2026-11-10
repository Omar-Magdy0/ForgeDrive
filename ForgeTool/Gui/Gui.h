#pragma once
#include <imgui.h>
#include <implot.h>

class Gui
{
public:
enum class MainPage
{
    Connections,
    Devices,
    Settings,
    About
};

struct State
{
    MainPage current_page = MainPage::Connections;
};


private:
    /* data */
    State state;
public:
    void render();
    void DrawSideMenu();
    void DrawTopMenu();
    void DrawBottomMenu();
    void DrawMainPanel();
    void init();
};
