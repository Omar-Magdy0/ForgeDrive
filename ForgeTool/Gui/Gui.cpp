#include "Gui.h"
#include <math.h>
#include <iostream>




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



