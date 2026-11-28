#include "ForgeTool.h"
#include "fdmc.h"
// --------------------------------------------------
// Connections list
// --------------------------------------------------
constexpr GridConfig card_grid   = {.minItemWidth = 160, .spacingX = 12, .spacingY = 12, .maxColumns = 6};
constexpr int        card_height = 170;
void                 ConnectionPage::DrawLaunchBar(IComm* comm, std::function<void()> Disconnect) {
  auto  avail        = ImGui::GetContentRegionAvail();
  float spacing      = ImGui::GetStyle().ItemSpacing.x;
  float buttonWidth  = (avail.x - spacing * 2) / 3.0f;
  float buttonHeight = ImGui::GetFrameHeight();
  ImGui::PushFont(fg.Style().fonts.IconM);
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + avail.y - buttonHeight);
  if (ImGui::Button(ICON_FA_LINK_SLASH, ImVec2(buttonWidth, 0))) Disconnect();
  ImGui::SameLine();
  ImGui::Button(ICON_FA_CIRCLE_INFO, ImVec2(buttonWidth, 0));
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_ARROW_UP_RIGHT_FROM_SQUARE, ImVec2(buttonWidth, 0))) {
    std::string name = "SAD";
    LaunchFdmc& laun = fg.Launch<LaunchFdmc>(name);
    laun.SetComm(comm);
    fg.SetStatus(fg.Style().colors.Info, "Launch " + laun.getWindowName());
  }
  ImGui::PopFont();
}
void ConnectionPage::DrawUsbCard(const UsbDeviceInfo& info, ImVec2 dim, IComm* comm, bool launchBar) {
  bool selected = false;
  if (CardBegin(ImGui::GetID(info._libusb_device), fg.Style(), selected, dim, 1)) {
    // Header
    ImGui::TextWrapped("%s", info.product.c_str());
    ImGui::Separator();
    auto device = Usbxch::FindConnection(info);
    if (device) {
      ImGui::PushFont(fg.Style().fonts.S);
      ImGui::TextColored(fg.Style().colors.True, "Connected");
      ImGui::PopFont();
    }
    ImGui::Text("%s", info.manufacturer.c_str());
    ImGui::Text("%s", info.serialNumber.c_str());
    ImGui::PushFont(fg.Style().fonts.S);
    ImGui::Text("%d:%d", info.vid, info.pid);
    ImGui::PopFont();
    if (!device) {
      float button_width  = 100.0f;
      float button_height = ImGui::GetFrameHeight();
      SetNextItemAlignment(button_width, button_height, AlignX::Center, AlignY::Bottom);
      if (ImGui::Button("Connect", ImVec2(button_width, button_height))) {
        {
          std::string error;
          Usbxch::Connect(info, error);
          bool ne    = error.empty();
          auto color = ne ? fg.Style().colors.Success : fg.Style().colors.Error;
          auto text  = "USB CONNECTION : " + (ne ? "SUCCESS" : error);
          fg.SetStatus(color, text);
        };
      }
    } else {
      if (launchBar) {
        DrawLaunchBar(comm, [device]() { Usbxch::Disconnect(device); });
      }
    }
  }
  CardEnd();
}
void ConnectionPage::DrawTCPCard(TcpClient* client, ImVec2 dim, bool launchBar = false) {
  std::string host;
  uint16_t    port;
  client->getAddress(host, port);
  auto address  = (host + ":" + std::to_string(port)).c_str();
  bool selected = false;
  if (CardBegin(ImGui::GetID(address), fg.Style(), selected, dim, 1)) {
    ImGui::PushFont(fg.Style().fonts.N);
    ImGui::TextWrapped("%s", address);
    ImGui::PopFont();
    ImGui::Separator();
    ImGui::PushFont(fg.Style().fonts.S);
    ImGui::TextColored(fg.Style().colors.True, "Connected");
    ImGui::PopFont();
    if (launchBar) {
      DrawLaunchBar(client, [client]() { TcpClient::Disconnect(client); });
    }
  }
  CardEnd();
}
void ConnectionPage::ConnectionsList() {
  // Traverse over active Communication port connections
  //  USB traverse
  auto usb_devices     = Usbxch::GetConnectedDevices();
  auto tcp_connections = TcpClient::GetConnections();
  if (GridBegin("Connections", card_grid)) {
    for (auto& device : usb_devices) {
      if (GridItem()) DrawUsbCard(device->GetDeviceInfo(), ImVec2(0, card_height), device, true);
    }
    for (auto* client : tcp_connections) {
      if (GridItem()) DrawTCPCard(client, ImVec2(0, card_height), true);
    }
    GridEnd();
  }
}
// --------------------------------------------------
// USB connection configuration
// --------------------------------------------------
void ConnectionPage::ConnectionSetup_Usb() {
  auto devices = Usbxch::GetDiscoveredDevices();
  if (ImGui::Button("Scan")) { devices = Usbxch::Scan(); }
  ImGui::Separator();
  if (GridBegin("UsbConnections", card_grid)) {
    for (auto& device : devices) {
      if (GridItem()) DrawUsbCard(device, ImVec2(0, card_height));
    }
    GridEnd();
  }
}
// --------------------------------------------------
// TCP connection configuration
// --------------------------------------------------
void ConnectionPage::ConnectionSetup_TCP() {
  ImGui::SetNextItemWidth(120);
  ImGui::InputText("IP Address", tcpAddress, sizeof(tcpAddress));
  ImGui::SetNextItemWidth(120);
  ImGui::InputInt("Port", &tcpPort);
  tcpPort = std::clamp(tcpPort, 1, 65535);
  if (ImGui::Button("Connect")) {
    std::string error;
    TcpClient::Connect(tcpAddress, static_cast<uint16_t>(tcpPort), error);
    bool ne    = error.empty();
    auto color = ne ? fg.Style().colors.Success : fg.Style().colors.Error;
    auto text  = "TCP CONNECTION : " + (ne ? "SUCCESS" : error);
    fg.SetStatus(color, text);
  }
}
// --------------------------------------------------
// Connections page
// --------------------------------------------------
void ConnectionPage::Render() {
  // ---------- Connection setup ----------
  auto avail = ImGui::GetContentRegionAvail();
  ImGui::PushFont(fg.Style().fonts.H2);
  ImGui::Text("Connection Setup");
  ImGui::PopFont();
  auto size = ImVec2(0, avail.y * 0.4);
  if (ImGui::BeginTabBar("TransportTabs")) {
    if (ImGui::BeginTabItem("USB")) {
      type = Type::USB;
      ImGui::BeginChild("ConnectionSetup", size);
      ConnectionSetup_Usb();
      ImGui::EndChild();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("TCP")) {
      type = Type::TCP;
      ImGui::BeginChild("ConnectionSetup", size);
      ConnectionSetup_TCP();
      ImGui::EndChild();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
  // ---------- Active connections ----------
  ImGui::PushFont(fg.Style().fonts.H2);
  ImGui::Text("Established Connections");
  ImGui::Separator();
  ImGui::PopFont();
  ImGui::BeginChild("ConnectionsList", ImVec2(0, 0));
  ConnectionsList();
  ImGui::EndChild();
}
