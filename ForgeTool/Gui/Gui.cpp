#include "Gui.h"
#include <math.h>
#include <iostream>
#include "IconsFontAwesome7.h"

#define BOTTOM_MENU_H 36
#define SIDE_MENU_W 48

Gui::Fonts Gui::fonts{};
Gui::ColorPalette Gui::colors{};

void Gui::render()
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin(
        "ForgeDrive",
        nullptr,
        ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse);

    DrawMenuBar();
    DrawSideMenu();
    ImGui::SameLine(0,5);
    DrawMainPanel();
    DrawBottomMenu();
    ImGui::End();
}

void Gui::DrawSideMenu()
{
    float height = ImGui::GetWindowHeight();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IMGUI_RGB(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
    ImGui::BeginChild(
        "SideMenu",
        ImVec2(SIDE_MENU_W, height - BOTTOM_MENU_H),
        ImGuiChildFlags_Borders,
        ImGuiWindowFlags_NoResize);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    struct MenuItem
    {
        MainPage page;
        const char *icon;
        const char *tooltip;
    };

    static constexpr MenuItem items[] =
        {
            {MainPage::Connections, ICON_FA_PLUG, "Connections"},
            {MainPage::Devices, ICON_FA_MICROCHIP, "Devices"},
            {MainPage::Settings, ICON_FA_GEAR, "Settings"},
            {MainPage::About, ICON_FA_CIRCLE_INFO, "About"}
        };

    ImVec2 avail = ImGui::GetContentRegionAvail();
    for (const auto &item : items)
    {
        ImGui::PushID(static_cast<int>(item.page));
        bool selected = state.current_page == item.page;
        ImGui::PushFont(fonts.IconM);
        if (ImGui::Selectable(item.icon, selected, ImGuiSelectableFlags_None, ImVec2(avail.x, 48.0f)))
            state.current_page = item.page;
        ImGui::PopFont();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", item.tooltip);
        ImGui::PopID();
    }
    ImGui::EndChild();
}

void Gui::DrawMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        // ---------- File ----------
        if (ImGui::BeginMenu("File"))
        {
            ImGui::EndMenu();
        }
        // ---------- View ----------
        if (ImGui::BeginMenu("View"))
        {
            ImGui::EndMenu();
        }
        // ---------- Tools ----------
        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::EndMenu();
        }
        // ---------- Right-side status ----------
        const float right_width = 180.0f;
        ImGui::SameLine(
            ImGui::GetWindowWidth() - right_width);
        ImGui::TextDisabled("ForgeTool");
        ImGui::EndMainMenuBar();
    }
}
void Gui::DrawBottomMenu()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IMGUI_HEX(0x263B50, 0.75));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
    ImGui::BeginChild("BottomMenu", ImVec2(0, BOTTOM_MENU_H), ImGuiChildFlags_None , ImGuiWindowFlags_NoResize);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    auto avail = ImGui::GetContentRegionAvail();
    auto* draw = ImGui::GetWindowDrawList();
    float y = (avail.y - ImGui::GetTextLineHeight()) * 0.5f;
    ImGui::SetCursorPos(ImVec2(10, y));
    auto pos = ImGui::GetCursorScreenPos();
    draw->AddCircleFilled(ImVec2(pos.x + 5, pos.y + ImGui::GetTextLineHeight() * 0.5f), 4.0f, ImGui::GetColorU32(status.color));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 16);
    ImGui::PushFont(fonts.S);
    ImGui::TextUnformatted(status.text.c_str());
    ImGui::PopFont();
    ImGui::EndChild();
}
void Gui::DrawMainPanel()
{
    float height = ImGui::GetWindowHeight();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IMGUI_RGB(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
    ImGui::BeginChild("MainPanel", ImVec2(0, height - BOTTOM_MENU_H), ImGuiChildFlags_None , ImGuiWindowFlags_NoResize);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
    switch (state.current_page)
    {
    case MainPage::Connections:
        DrawConnectionsPage();
        break;

    case MainPage::Devices:
        // DrawDevicesPage(ctx);
        break;

    case MainPage::Settings:
        // DrawSettingsPage(ctx);
        break;

    case MainPage::About:
        // DrawAboutPage(ctx);
        break;
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
}

void ApplyDarkTheme()
{
    ImGuiStyle &style = ImGui::GetStyle();
    // --------------------------------------------------
    // Core surfaces
    // --------------------------------------------------
    style.Colors[ImGuiCol_Text] = ImVec4(0.88f, 0.90f, 0.93f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.43f, 0.47f, 0.53f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.075f, 0.085f, 0.105f, 1.00f);
    style.Colors[ImGuiCol_ChildBg] = Gui::colors.A2;
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.105f, 0.120f, 0.145f, 0.98f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.20f, 0.23f, 0.28f, 0.75f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    // --------------------------------------------------
    // Frames (Input, Checkbox, Slider backgrounds)
    // --------------------------------------------------
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.30f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.19f, 0.24f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.19f, 0.23f, 0.30f, 1.00f);
    // --------------------------------------------------
    // Title bars
    // --------------------------------------------------
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.065f, 0.075f, 0.095f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.085f, 0.100f, 0.125f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.065f, 0.075f, 0.095f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.085f, 0.095f, 0.115f, 1.00f);
    // --------------------------------------------------
    // Scrollbars
    // --------------------------------------------------
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.065f, 0.075f, 0.090f, 0.50f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.28f, 0.34f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.32f, 0.38f, 0.46f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.48f, 0.58f, 1.00f);
    // --------------------------------------------------
    // Interactive accent (Electric Blue)
    // --------------------------------------------------
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.30f, 0.62f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_CheckboxSelectedBg] = ImVec4(0.18f, 0.38f, 0.65f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.25f, 0.53f, 0.88f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.35f, 0.66f, 1.00f, 1.00f);
    // --------------------------------------------------
    // Buttons
    // --------------------------------------------------
    style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.2f, 0.5f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.34f, 0.52f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.43f, 0.75f, 1.00f);
    // --------------------------------------------------
    // Headers / Selectables / Tree nodes
    // --------------------------------------------------
    style.Colors[ImGuiCol_Header] = ImVec4(0.15f, 0.20f, 0.27f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.19f, 0.31f, 0.46f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.18f, 0.39f, 0.65f, 1.00f);
    // --------------------------------------------------
    // Separators
    // --------------------------------------------------
    style.Colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.23f, 0.28f, 0.75f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.48f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.30f, 0.62f, 1.00f, 1.00f);
    // --------------------------------------------------
    // Resize grips
    // --------------------------------------------------
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.20f, 0.35f, 0.52f, 0.35f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.25f, 0.53f, 0.85f, 0.75f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.30f, 0.62f, 1.00f, 1.00f);
    // --------------------------------------------------
    // Input cursor
    // --------------------------------------------------
    style.Colors[ImGuiCol_InputTextCursor] = ImVec4(0.35f, 0.67f, 1.00f, 1.00f);
    // --------------------------------------------------
    // Tabs
    // --------------------------------------------------
    style.Colors[ImGuiCol_Tab]                       = IMGUI_HEX(0x263440, 1.00f);
    style.Colors[ImGuiCol_TabHovered]                = IMGUI_HEX(0x344957, 1.00f);
    style.Colors[ImGuiCol_TabSelected]               = IMGUI_HEX(0x405070, 1.00f);
    style.Colors[ImGuiCol_TabSelectedOverline]       = IMGUI_HEX(0x00BFFF, 1.00f);
    style.Colors[ImGuiCol_TabDimmed]                 = IMGUI_HEX(0x202A34, 1.00f);
    style.Colors[ImGuiCol_TabDimmedSelected]         = IMGUI_HEX(0x263440, 1.00f);
    style.Colors[ImGuiCol_TabDimmedSelectedOverline] = IMGUI_HEX(0x405070, 1.00f);
    // --------------------------------------------------
    // Plots
    // --------------------------------------------------
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.65f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.55f, 0.78f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.25f, 0.52f, 0.82f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.38f, 0.68f, 1.00f, 1.00f);
    // --------------------------------------------------
    // Tables
    // --------------------------------------------------
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.13f, 0.16f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.25f, 0.29f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.17f, 0.20f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.15f, 0.18f, 0.22f, 0.35f);
    // --------------------------------------------------
    // Links and text selection
    // --------------------------------------------------
    style.Colors[ImGuiCol_TextLink] = ImVec4(0.25f, 0.48f, 0.8f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.40f, 0.60f, 0.50f);
    // --------------------------------------------------
    // Tree lines and drag/drop
    // --------------------------------------------------
    style.Colors[ImGuiCol_TreeLines] = ImVec4(0.25f, 0.30f, 0.37f, 0.60f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.35f, 0.68f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_DragDropTargetBg] = ImVec4(0.20f, 0.42f, 0.70f, 0.20f);
    style.Colors[ImGuiCol_UnsavedMarker] = ImVec4(1.00f, 0.65f, 0.20f, 1.00f);
    // --------------------------------------------------
    // Navigation
    // --------------------------------------------------
    style.Colors[ImGuiCol_NavCursor] = ImVec4(0.35f, 0.68f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.80f, 0.88f, 1.00f, 0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.05f, 0.06f, 0.08f, 0.65f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.02f, 0.025f, 0.04f, 0.70f);


    // ---------- Surfaces / Cards ----------
    Gui::colors.A1          = IMGUI_HEX(0x0C1116, 1.00f);
    Gui::colors.A1Hover     = IMGUI_HEX(0x1C2B36, 1.00f);
    Gui::colors.A1Selected  = IMGUI_HEX(0x203642, 1.00f);
    Gui::colors.A1Border    = IMGUI_HEX(0x29404D, 1.00f);
    Gui::colors.A2          = IMGUI_HEX(0x202832, 1.00f);
    Gui::colors.A2Hover     = IMGUI_HEX(0x26323E, 1.00f);
    Gui::colors.A2Selected  = IMGUI_HEX(0x2B3B49, 1.00f);
    Gui::colors.A2Border    = IMGUI_HEX(0x354653, 1.00f);
}
void ApplyGeometry()
{
    ImGuiStyle &style = ImGui::GetStyle();
    // Window / card geometry
    style.WindowPadding = ImVec2(0, 0);
    style.WindowRounding = 8.0f;
    style.WindowBorderSize = 1.0f;
    // Child panels / cards
    style.ChildRounding = 8.0f;
    style.ChildBorderSize = 0.5f;
    style.PopupRounding = 7.0f;
    style.PopupBorderSize = 1.0f;
    // Widgets
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.FrameRounding = 5.0f;
    style.FrameBorderSize = 0.0f;
    // Buttons
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    // Layout
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.CellPadding = ImVec2(8.0f, 6.0f);
    // Separators
    style.SeparatorTextBorderSize = 1.0f;
    // Scrollbars
    style.ScrollbarSize = 12.0f;
    style.ScrollbarRounding = 8.0f;
    // Grab handles
    style.GrabMinSize = 10.0f;
    style.GrabRounding = 5.0f;
    // Selectable
    style.SelectableTextAlign = ImVec2(0.5f, 0.5f);
}
void ApplyFonts()
{
    ImGuiIO &io = ImGui::GetIO();

    // Font Awesome
    ImFontConfig icon_config;
    icon_config.PixelSnapH = true;
    static const ImWchar icon_ranges[] = {
        ICON_MIN_FA,
        ICON_MAX_FA,
        0};

    auto FONT_REGULAR = FONTS_DIR "/DejaVu/DejaVuSans.ttf";
    auto FONT_BOLD    = FONTS_DIR "/DejaVu/DejaVuSans-Bold.ttf";
    auto FONT_ITALIC  = FONTS_DIR "/DejaVu/DejaVuSans-Oblique.ttf";

    Gui::fonts.H1 = io.Fonts->AddFontFromFileTTF(FONT_REGULAR, 26.0f);
    Gui::fonts.H2 = io.Fonts->AddFontFromFileTTF(FONT_REGULAR, 22.0f);
    Gui::fonts.H3 = io.Fonts->AddFontFromFileTTF(FONT_REGULAR, 18.0f);
    Gui::fonts.N = io.Fonts->AddFontFromFileTTF(FONT_REGULAR, 16.0f);
    Gui::fonts.N_bold = io.Fonts->AddFontFromFileTTF(FONT_BOLD, 16.0f);
    Gui::fonts.N_italic = io.Fonts->AddFontFromFileTTF(FONT_ITALIC, 16.0f);
    Gui::fonts.S = io.Fonts->AddFontFromFileTTF(FONT_REGULAR, 13.0f);
    Gui::fonts.S_bold = io.Fonts->AddFontFromFileTTF(FONT_BOLD, 13.0f);
    Gui::fonts.S_italic = io.Fonts->AddFontFromFileTTF(FONT_ITALIC, 13.0f);
    Gui::fonts.IconL = io.Fonts->AddFontFromFileTTF(FONTS_DIR "/FontAwesome7/Font Awesome 7 Free-Solid-900.otf", 24.0f, &icon_config, icon_ranges);
    Gui::fonts.IconM = io.Fonts->AddFontFromFileTTF(FONTS_DIR "/FontAwesome7/Font Awesome 7 Free-Solid-900.otf", 18.0f, &icon_config, icon_ranges);
    Gui::fonts.IconS = io.Fonts->AddFontFromFileTTF(FONTS_DIR "/FontAwesome7/Font Awesome 7 Free-Solid-900.otf", 14.0f, &icon_config, icon_ranges);
}

void Gui::init()
{
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();
    ImPlot::CreateContext();


    ImGuiStyle &style = ImGui::GetStyle();
    ApplyGeometry();
    ApplyDarkTheme();
    ApplyFonts();
    ImGui::PushFont(fonts.N);
}
