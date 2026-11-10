#include "Gui.h"
#include <math.h>
#include <iostream>
#include "IconsFontAwesome7.h"

#define TOP_MENU_H 24
#define BOTTOM_MENU_H 24
#define SIDE_MENU_W 64

void Gui::render()
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

    ImGui::Begin(
        "ForgeDrive",
        nullptr,
        ImGuiWindowFlags_NoTitleBar |
           ImGuiWindowFlags_NoCollapse);
    
    ImGui::BeginChild("Body", ImVec2(0, -BOTTOM_MENU_H), ImGuiChildFlags_None);
    DrawSideMenu();
    ImGui::SameLine(0, 0);
    // Main content
    DrawMainPanel();
    ImGui::EndChild();
    DrawBottomMenu();
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void Gui::DrawSideMenu()
{
    ImGui::BeginChild(
        "SideMenu",
        ImVec2(SIDE_MENU_W, 0),
        ImGuiChildFlags_Borders
    );

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

    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
    ImVec2 avail = ImGui::GetContentRegionAvail();

    for (const auto &item : items)
    {
        ImGui::PushID(static_cast<int>(item.page));

        bool selected = state.current_page == item.page;

        if (ImGui::Selectable(
                item.icon,
                selected,
                ImGuiSelectableFlags_None,
                ImVec2(avail.x, 48.0f)))
        {
            state.current_page = item.page;
        }

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", item.tooltip);

        ImGui::PopID();
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
}

void Gui::DrawBottomMenu()
{
    ImGui::BeginChild("BottomMenu", ImVec2(0, BOTTOM_MENU_H));

    ImGui::Text("USBXCH");

    ImGui::SameLine();

    ImGui::TextColored(
        ImVec4(0.2f, 0.9f, 0.3f, 1.0f),
        "Connected");

    ImGui::EndChild();
}

void Gui::DrawMainPanel()
{
    switch (state.current_page)
    {
    case MainPage::Connections:
        // DrawConnectionsPage(ctx);
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
}


void Gui::init()
{

}
