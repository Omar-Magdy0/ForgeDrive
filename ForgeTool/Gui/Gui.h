#pragma once





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
 

private:
    /* data */
    State state;
    void DrawSideMenu();
    void DrawMenuBar();
    void DrawBottomMenu();
    void DrawMainPanel();
    void DrawConnectionsPage();
public:

    StatusMessage status;
    ConnectionsPageState connectionPageState;
    void render();
};


