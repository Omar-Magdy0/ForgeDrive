#pragma once
#include <imgui.h>
#include <implot.h>
#include <string>
#define IMGUI_RGB(r, g, b, a) ImVec4( \
    (r) / 255.0f,                     \
    (g) / 255.0f,                     \
    (b) / 255.0f,                     \
    a)

#define IMGUI_HEX(hex, a) ImVec4(  \
    ((hex >> 16) & 0xFF) / 255.0f, \
    ((hex >> 8) & 0xFF) / 255.0f,  \
    ((hex) & 0xFF) / 255.0f,       \
    a)






struct ConnectionsPageState
{
    enum class ConnectionType
    {
        USB,
        TCP,
        END
    };
    const char* ConnectionType_name[(unsigned int)ConnectionType::END] = { "USB", "TCP" };
    ConnectionType type = ConnectionType::USB;
    char tcp_address[64] = "192.168.1.100";
    int tcp_port = 5001;
};


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
    struct StatusMessage {
        std::string text;
        ImVec4 color;
    };
    struct Fonts
    {
        ImFont *H1;
        ImFont *H2;
        ImFont *H3;
        ImFont *N;
        ImFont *N_bold;
        ImFont *N_italic;
        ImFont *S;
        ImFont *S_bold;
        ImFont *S_italic;
        ImFont *IconL;
        ImFont *IconM;
        ImFont *IconS;
    };
    struct ColorPalette
    {
        // ---------- Boolean ----------
        ImVec4 True        = IMGUI_HEX(0x22C55E, 1);
        ImVec4 False       = IMGUI_HEX(0xEF4444, 1);
        ImVec4 Unknown     = IMGUI_HEX(0x9CA3AF, 1);

        // ---------- Status ----------
        ImVec4 Success     = IMGUI_HEX(0x22C55E, 1);
        ImVec4 Error       = IMGUI_HEX(0xEF4444, 1);
        ImVec4 Warning     = IMGUI_HEX(0xF59E0B, 1);
        ImVec4 Info        = IMGUI_HEX(0x3B82F6, 1);
        ImVec4 Neutral     = IMGUI_HEX(0x9CA3AF, 1);

        // ---------- Connection ----------
        ImVec4 Connected   = IMGUI_HEX(0x22C55E, 1);
        ImVec4 Disconnected= IMGUI_HEX(0x6B7280, 1);
        ImVec4 Connecting  = IMGUI_HEX(0xF59E0B, 1);

        // ---------- Text ----------
        ImVec4 TextPrimary   = IMGUI_HEX(0xE5E7EB, 1);
        ImVec4 TextSecondary = IMGUI_HEX(0x9CA3AF, 1);
        ImVec4 TextMuted     = IMGUI_HEX(0x6B7280, 1);
        ImVec4 TextDisabled  = IMGUI_HEX(0x4B5563, 1);

        // ---------- Accent ----------
        ImVec4 Primary      = IMGUI_HEX(0x00BFFF, 1);
        ImVec4 Secondary    = IMGUI_HEX(0x8B5CF6, 1);

        // ---------- Interaction ----------
        ImVec4 Hover        = IMGUI_HEX(0xFFFFFF, 0.08);
        ImVec4 Active       = IMGUI_HEX(0xFFFFFF, 0.14);
        ImVec4 Selected     = IMGUI_HEX(0x00BFFF, 0.18);
        ImVec4 Disabled     = IMGUI_HEX(0x6B7280, 0.35);

        // ---------- Surfaces / Cards ----------
        // ---------- Surfaces / Cards ----------
        ImVec4 A1          = IMGUI_HEX(0x18232C, 1.00f);
        ImVec4 A1Hover     = IMGUI_HEX(0x1C2B36, 1.00f);
        ImVec4 A1Selected  = IMGUI_HEX(0x203642, 1.00f);
        ImVec4 A1Border    = IMGUI_HEX(0x29404D, 1.00f);

        ImVec4 A2          = IMGUI_HEX(0x202832, 1.00f);
        ImVec4 A2Hover     = IMGUI_HEX(0x26323E, 1.00f);
        ImVec4 A2Selected  = IMGUI_HEX(0x2B3B49, 1.00f);
        ImVec4 A2Border    = IMGUI_HEX(0x354653, 1.00f);
    };



private:
    /* data */
    State state;
public:
    static Fonts fonts;
    static ColorPalette colors;
    StatusMessage status;
    ConnectionsPageState connectionPageState;
    void render();
    void DrawSideMenu();
    void DrawMenuBar();
    void DrawBottomMenu();
    void DrawMainPanel();
    void DrawConnectionsPage();
    void init();
};
