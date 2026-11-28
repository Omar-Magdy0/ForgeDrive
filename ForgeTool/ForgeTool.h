#pragma once
#include "Style.h"
#include "TcpClient.h"
#include "Usbxch.h"
#include <GLFW/glfw3.h>
#include <assert.h>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <queue>
#include <string>
#include <thread>
// Callback to handle GLFW errors
static inline void glfw_error_callback(int error, const char* description) { std::cerr << "GLFW Error " << error << ": " << description << std::endl; }
class ForgeTool;
class ConnectionPage;
class Launchable;
class ConnectionPage {
public:
  enum class Type { USB, TCP, END };
  const char* typeName[(unsigned int)Type::END] = {"USB", "TCP"};
  Type        type                              = Type::USB;
  char        tcpAddress[64]                    = "127.0.0.1";
  int         tcpPort                           = 5001;
  ForgeTool&  fg;
  ConnectionPage(ForgeTool& fg_) : fg(fg_) {};
  void DrawLaunchBar(IComm* comm, std::function<void()> disconnect);
  void DrawUsbCard(const UsbDeviceInfo& info, ImVec2 dim, IComm* comm = nullptr, bool launchBar = false);
  void DrawTCPCard(TcpClient* client, ImVec2 dim, bool launchBar);
  void ConnectionsList();
  void ConnectionSetup_Usb();
  void ConnectionSetup_TCP();
  void Render();
};
struct LaunchablePage {
  void Render();
};
class ForgeTool {
private:
  enum class MainPage { Connections, Devices, Settings, Launchables, About };
  struct State {
    std::string statusTxt;
    ImVec4      statusCol;
    MainPage    currentPage = MainPage::Connections;
  };
  State                                    pState; // Pending state
  State                                    cState; // Current populated state
  std::mutex                               stateMutex;
  LaunchablePage                           launchablePage;
  ConnectionPage                           connectionPage;
  std::vector<std::unique_ptr<Launchable>> launchables;
  ImGuiContext*                            guiCtx;
  ImPlotContext*                           pltCtx;
  GLFWwindow*                              window;
  ForgeStyle                               style;
  int                                      displayWidth  = 1200;
  int                                      displayHeight = 720;
  void                                     Render();
  void                                     DrawSideMenu();
  void                                     DrawMenuBar();
  void                                     DrawBottomMenu();
  void                                     DrawMainPanel();
  bool                                     execRunning = false;
  std::mutex                               execMutex;
  std::thread                              execThread;
  std::condition_variable                  execCv;
  std::queue<std::function<void()>>        execQueue;
public:
  ForgeTool();
  ~ForgeTool();
  const ForgeStyle&                          Style() const { return style; }
  void                                       SetStatus(const ImVec4& statusCol, const std::string& statusTxt);
  void                                       Exec(std::function<void()> task);
  template <typename T, typename... Args> T& Launch(Args&&... args) {
    auto instance = std::make_unique<T>(*this, std::forward<Args>(args)...);
    T&   ref      = *instance;
    launchables.push_back(std::move(instance));
    return ref;
  }
};
class Launchable {
protected:
  ForgeTool&        fg;
  const std::string windowName;
  bool              open      = true;
  bool              visible   = true;
  bool              minimized = false;
  bool              docked    = false;
  ImGuiViewport*    viewport;
public:
  Launchable(ForgeTool& fg_, std::string launchableName_, std::string name_)
      : fg(fg_), windowName(launchableName_ + ":" + name_ + "##" + std::to_string(reinterpret_cast<std::uintptr_t>(this))) {}
  virtual ~Launchable()           = default;
  virtual void       RenderImpl() = 0;
  void               Render();
  bool               ShouldClose() const { return !open; }
  const std::string& getWindowName() { return windowName; }
  const ForgeStyle&  Style() const { return fg.Style(); }
  void               Exec(std::function<void()> task) { return fg.Exec(task); }
};