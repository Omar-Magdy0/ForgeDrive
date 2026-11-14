#include "Usbxch.h"
#include "TCPClient.h"
#include "imgui.h"
#include <cstdio>
#include <cstdint>
#include <string>
#include <functional>


// --------------------------------------------------
// Connections list
// --------------------------------------------------
void DrawLaunchBar(float width, float height,
                   std::function<void()> disconnect,
                   std::function<void()> about,
                   std::function<void()> launch)
{
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float buttonWidth = (width - spacing * 2) / 3.0f;
    ImGui::PushFont(Gui::fonts.IconM);
    if (ImGui::Button(ICON_FA_LINK_SLASH, ImVec2(buttonWidth, height)))
        if (disconnect) disconnect();
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_CIRCLE_INFO, ImVec2(buttonWidth, height)))
        if (about) about();
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ARROW_UP_RIGHT_FROM_SQUARE, ImVec2(buttonWidth, height)))
        if (launch) launch();
    ImGui::PopFont();
}


void DrawUsbCard(UsbDeviceInfo info, ImVec2 dim, Gui &gui, bool launchBar = false)
{
    ImGui::BeginChild(ImGui::GetID(info._libusb_device), dim, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
    // Header
    ImGui::PushFont(Gui::fonts.H3);
    if(info.product.size() > 0)
        ImGui::TextWrapped(info.product.c_str());
    else 
        ImGui::TextWrapped("Unidentified:%d:%d", info.vid, info.pid);
    ImGui::PopFont();
    ImGui::Separator();
    auto device = Usbxch::findConnection(info);
    if(device)
    {
        ImGui::PushFont(Gui::fonts.S);
        ImGui::TextColored(Gui::colors.True,"Connected");
        ImGui::PopFont();
    }
    ImGui::Text(info.manufacturer.c_str());
    ImGui::Text(info.serial_number.c_str());
    ImGui::PushFont(Gui::fonts.S);
    ImGui::Text("%d:%d", info.vid, info.pid);
    ImGui::PopFont();
    if(!device)
    {
        float button_width = 100.0f;
        float button_height = ImGui::GetFrameHeight();
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + avail.y - button_height);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - button_width) * 0.5f);
        if(ImGui::Button("Connect", ImVec2(button_width, button_height)))
        {
            std::string error;
            Usbxch::connect(info, error);
            bool ne = error.empty();
            gui.status.color = ne ? gui.colors.Success : gui.colors.Error;
            gui.status.text = "USB CONNECTION : " + (ne? "SUCCESS" : error);
        }
    }else
    {
        if(launchBar)
        {
            auto avail = ImGui::GetContentRegionAvail();
            float buttonHeight = 40;
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + avail.y - buttonHeight);
            DrawLaunchBar(avail.x, buttonHeight, 
            [&]() {
                Usbxch::disconnect(device);
            },
            [&]() {
                // Show connection information //TODO:
            },
            [&]() {
                // Launch application
            });
        }        
    }
}


void DrawTCPCard(TCPClient *client, ImVec2 dim, Gui &gui, bool launchBar = false)
{
    std::string host;
    uint16_t port;
    client->getAddress(host, port);
    auto address = (host + ":" + std::to_string(port)).c_str();
    ImGui::BeginChild(address, dim, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
    ImGui::PushFont(Gui::fonts.N);
    ImGui::TextWrapped(address);
    ImGui::PopFont();
    ImGui::Separator();
    ImGui::PushFont(Gui::fonts.S);
    ImGui::TextColored(Gui::colors.True,"Connected");
    ImGui::PopFont();

    if(launchBar)
    {
        auto avail = ImGui::GetContentRegionAvail();
        float buttonHeight = 40;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + avail.y - buttonHeight);
        DrawLaunchBar(avail.x, buttonHeight, 
        [&]() {
            TCPClient::disconnect(client);
        },
        [&]() {
            // Show connection information //TODO:
        },
        [&]() {
            // Launch application
        });
    }

}

void EndCard()
{
    ImGui::EndChild();
};

void ConnectionsList(Gui &gui)
{
    ImGui::Indent(20.0f);
    auto pos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(pos.x, pos.y+10));
    ImGui::PushFont(Gui::fonts.H2);
    ImGui::Text("Connections");
    ImGui::Separator();
    ImGui::PopFont();
    ImGui::Unindent(20.0f);
    //Traverse over active Communication port connections
    // USB traverse
    auto avail = ImGui::GetContentRegionAvail();
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float width = (avail.x - spacing * 3) / 4.0f;
    int c = 0;
    auto usb_devices = Usbxch::getConnectedDevices();
    for (auto& device : usb_devices)
    {
        if (c % 4 != 0)
            ImGui::SameLine();
        DrawUsbCard(device->getDeviceInfo(), ImVec2(width, 160), gui, true);
        EndCard();
        c++;
    }
    auto tcp_connections = TCPClient::getConnections();
    for (auto* client : tcp_connections)
    {
        if (c % 4 != 0)
            ImGui::SameLine();
        DrawTCPCard(client, ImVec2(width, 160), gui, true);
        EndCard();
        c++;
    }
}
// --------------------------------------------------
// USB connection configuration
// --------------------------------------------------
void ConnectionSetup_Usb(Gui &gui)
{
    ImGui::Indent(20.0f);
    ImGui::PushFont(Gui::fonts.H3);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("USB Devices");
    ImGui::PopFont();
    ImGui::SameLine();
    auto devices = Usbxch::getDiscoveredDevices();
    if (ImGui::Button("Scan"))
    {
        devices = Usbxch::scan();
    }
    ImGui::Separator();
    ImGui::Unindent(20.0f);
    auto avail = ImGui::GetContentRegionAvail();
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float width = (avail.x - spacing * 3) / 4.0f;
    for(int i = 0; i < devices.size(); i++)
    {
        DrawUsbCard(devices[i], ImVec2(width, 180), gui);
        EndCard();
        if ((i + 1) % 4 != 0 && i + 1 < devices.size())
            ImGui::SameLine();
    }
}

// --------------------------------------------------
// TCP connection configuration
// --------------------------------------------------
void ConnectionSetup_TCP(Gui &gui)
{
    ImGui::Indent(20.0f);
    ImGui::Text("TCP Connection");
    ImGui::Separator();
    ImGui::SetNextItemWidth(150);
    ImGui::InputText("IP Address", gui.connectionPageState.tcp_address, sizeof(gui.connectionPageState.tcp_address));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    ImGui::InputInt("Port", &gui.connectionPageState.tcp_port);
    ImGui::SameLine();
    gui.connectionPageState.tcp_port = std::clamp(gui.connectionPageState.tcp_port, 1, 65535);
    if (ImGui::Button("Connect", ImVec2(120, 0)))
    {
        std::string error;
        TCPClient::connect(gui.connectionPageState.tcp_address, static_cast<uint16_t>(gui.connectionPageState.tcp_port), error);
        bool ne = error.empty();
        gui.status.color = ne ? gui.colors.Success : gui.colors.Error;
        gui.status.text = "TCP CONNECTION : " + (ne? "SUCCESS" : error);
    }
    ImGui::Unindent(20.0f);
}

// --------------------------------------------------
// Connections page
// --------------------------------------------------
void Gui::DrawConnectionsPage()
{
    // ---------- Connection setup ----------
    ImVec2 avail;
    avail = ImGui::GetContentRegionAvail();
    
    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors.A1);
    ImGui::BeginChild("ConnectionSetup", ImVec2(0, avail.y*0.5));
    ImGui::PopStyleColor();
    if (ImGui::BeginTabBar("TransportTabs"))
    {
        if (ImGui::BeginTabItem("USB"))
        {
            connectionPageState.type = ConnectionsPageState::ConnectionType::USB;
            ConnectionSetup_Usb(*this);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("TCP"))
        {
            connectionPageState.type = ConnectionsPageState::ConnectionType::TCP;
            ConnectionSetup_TCP(*this);
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    ImGui::EndChild();
    // ---------- Active connections ----------
    ImGui::PushStyleColor(ImGuiCol_ChildBg, colors.A1);
    ImGui::BeginChild("ConnectionsList", ImVec2(0, 0));
    ImGui::PopStyleColor();
    
    ConnectionsList(*this);
    ImGui::EndChild();
}
