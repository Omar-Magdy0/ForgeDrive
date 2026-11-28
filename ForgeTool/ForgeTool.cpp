#include "ForgeTool.h"
#define BOTTOM_MENU_H 36
#define SIDE_MENU_W   48
int main() {
  Usbxch::Init(10);
  TcpClient::Init();
  { ForgeTool app; }
  Usbxch::DeInit();
  TcpClient::DeInit();
  return 0;
}
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <math.h>
ForgeTool::ForgeTool() : connectionPage(*this) {
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) return;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  const char* glsl_version = "#version 150";
  window                   = glfwCreateWindow(displayWidth, displayHeight, "ForgeTool", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  IMGUI_CHECKVERSION();
  guiCtx                          = ImGui::CreateContext();
  pltCtx                          = ImPlot::CreateContext();
  ImGuiIO& io                     = ImGui::GetIO();
  io.ConfigFlags                 |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags                 |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags                 |= ImGuiConfigFlags_ViewportsEnable;
  io.ConfigViewportsNoAutoMerge   = true;
  io.ConfigViewportsNoDecoration  = false;
  style.ApplyGeometry();
  ImGuiStyle& imstyle = ImGui::GetStyle();
  glfwSetWindowSizeLimits(window, imstyle.WindowMinSize.x, imstyle.WindowMinSize.x, GLFW_DONT_CARE, GLFW_DONT_CARE);
  style.ApplyTheme();
  style.ApplyFonts();
  io.FontDefault = style.fonts.N;
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);
  pState      = cState;
  execRunning = true;
  execThread  = std::thread([this]() {
    while (execRunning) {
      std::function<void()> task;
      {
        std::unique_lock lock(execMutex);
        execCv.wait(lock, [this]() { return !execQueue.empty() || !execRunning; });
        if (!execRunning && execQueue.empty()) break;
        task = std::move(execQueue.front());
        execQueue.pop();
      }
      if (task) {
        try {
          task();
        } catch (const std::exception& e) { std::cerr << "[GuiWindow] Task exception: " << e.what() << '\n'; }
      }
    }
  });
  while (!glfwWindowShouldClose(window)) {
    // Update cState from pState
    {
      std::lock_guard lock(stateMutex);
      cState = pState;
    }
    glfwPollEvents();
    glfwMakeContextCurrent(window);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    Render();
    for (auto it = launchables.begin(); it != launchables.end();) {
      auto& r = *it;
      if (r->ShouldClose()) {
        it = launchables.erase(it);
      } else {
        r->Render();
        ++it;
      }
    }
    ImGui::Render();
    glfwGetFramebufferSize(window, &displayWidth, &displayHeight);
    glViewport(0, 0, displayWidth, displayHeight);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      GLFWwindow* backup_current_context = glfwGetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(backup_current_context);
    }
    glfwSwapBuffers(window);
  }
}
ForgeTool::~ForgeTool() {
  // Cleanup
  {
    std::lock_guard lock(execMutex);
    execRunning = false;
  }
  execCv.notify_one();
  if (execThread.joinable()) execThread.join();
  launchables.clear();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
}
void ForgeTool::SetStatus(const ImVec4& statusCol, const std::string& statusTxt) {
  std::lock_guard lock(stateMutex);
  pState.statusTxt = statusTxt;
  pState.statusCol = statusCol;
}
void ForgeTool::Exec(std::function<void()> task) {
  {
    std::lock_guard lock(execMutex);
    execQueue.push(std::move(task));
  }
  execCv.notify_one();
}
void ForgeTool::Render() {
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::Begin("ForgeDrive", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoScrollWithMouse);
  DrawMenuBar();
  DrawSideMenu();
  ImGui::SameLine(0, 5);
  DrawMainPanel();
  DrawBottomMenu();
  ImGui::End();
}
void ForgeTool::DrawSideMenu() {
  const float padding = ImGui::GetStyle().WindowPadding.x;
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() - padding);
  ImGui::BeginGroup();
  struct MenuItem {
    MainPage    page;
    const char* icon;
    const char* tooltip;
  };
  static constexpr MenuItem items[] = {{MainPage::Connections, ICON_FA_PLUG, "Connections"},
                                       {MainPage::Devices, ICON_FA_MICROCHIP, "Devices"},
                                       {MainPage::Settings, ICON_FA_GEAR, "Settings"},
                                       {MainPage::Launchables, ICON_FA_UP_RIGHT_FROM_SQUARE, "Launchables"},
                                       {MainPage::About, ICON_FA_CIRCLE_INFO, "About"}};
  for (const auto& item : items) {
    bool selected = pState.currentPage == item.page;
    ImGui::PushID(static_cast<int>(item.page));
    ImGui::PushFont(style.fonts.IconM);
    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
    if (ImGui::Selectable(item.icon, selected, ImGuiSelectableFlags_None, ImVec2(SIDE_MENU_W + padding, 48.0f))) pState.currentPage = item.page;
    ImGui::PopFont();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", item.tooltip);
    ImGui::PopStyleVar();
    ImGui::PopID();
  }
  ImGui::EndGroup();
}
void ForgeTool::DrawMenuBar() {
  if (ImGui::BeginMenuBar()) {
    // ---------- File ----------
    if (ImGui::BeginMenu("File")) { ImGui::EndMenu(); }
    // ---------- View ----------
    if (ImGui::BeginMenu("View")) { ImGui::EndMenu(); }
    // ---------- Tools ----------
    if (ImGui::BeginMenu("Tools")) { ImGui::EndMenu(); }
    // ---------- Right-side status ----------
    const float right_width = 180.0f;
    ImGui::SameLine(ImGui::GetWindowWidth() - right_width);
    ImGui::TextDisabled("ForgeTool");
    ImGui::EndMenuBar();
  }
}
void ForgeTool::DrawBottomMenu() {
  ImGui::BeginGroup();
  const auto& style   = ImGui::GetStyle();
  auto*       draw    = ImGui::GetWindowDrawList();
  const float padding = style.WindowPadding.x;
  const float height  = BOTTOM_MENU_H;
  // Extend background across the parent's full content width.
  ImVec2 min  = ImGui::GetCursorScreenPos();
  min.x      -= padding;
  ImVec2 max(min.x + ImGui::GetWindowWidth(), min.y + height);
  draw->AddRectFilled(min, max, ImGui::GetColorU32(Style().colors.Secondary));
  // Position content inside the bottom menu.
  const float textHeight = ImGui::GetTextLineHeight();
  const float y          = (height - textHeight) * 0.5f;
  ImGui::SetCursorPos(ImVec2(padding + 10, ImGui::GetCursorPosY() + y));
  auto pos = ImGui::GetCursorScreenPos();
  draw->AddCircleFilled(ImVec2(pos.x + 5, pos.y + textHeight * 0.5f), 4.0f, ImGui::GetColorU32(cState.statusCol));
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 16);
  ImGui::PushFont(Style().fonts.S);
  ImGui::TextUnformatted(cState.statusTxt.c_str());
  ImGui::PopFont();
  ImGui::EndGroup();
}
void ForgeTool::DrawMainPanel() {
  float height = ImGui::GetContentRegionAvail().y;
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImRGB(0, 0, 0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
  ImGui::BeginChild("MainPanel", ImVec2(0, height - BOTTOM_MENU_H), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground);
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
  switch (cState.currentPage) {
  case MainPage::Connections: connectionPage.Render(); break;
  case MainPage::Devices: break;
  case MainPage::Settings: break;
  case MainPage::Launchables: launchablePage.Render(); break;
  case MainPage::About: break;
  }
  ImGui::EndChild();
}
void Launchable::Render() {
  if (ImGui::Begin(windowName.c_str(), &open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {
    viewport     = ImGui::GetWindowViewport();
    auto* native = static_cast<GLFWwindow*>(viewport->PlatformHandle);
    if (native) {
      ImGuiStyle& imstyle = ImGui::GetStyle();
      glfwSetWindowSizeLimits(native, imstyle.WindowMinSize.x, imstyle.WindowMinSize.x, GLFW_DONT_CARE, GLFW_DONT_CARE);
    }
    RenderImpl();
  }
  ImGui::End();
}
