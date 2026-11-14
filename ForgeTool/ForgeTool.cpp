#include "ForgeTool.h"


#define BOTTOM_MENU_H 36
#define SIDE_MENU_W 48

int main()
{
    ForgeTool app;
    Usbxch::init(10);
    TCPClient::init();
    app.init();
    app.run();
    app.deinit();
    Usbxch::deinit();
    TCPClient::deinit();
    return 0;
}


#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
void ForgeTool::onInit()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return;
#if defined(__APPLE__)
    const char *glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char *glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
    glfwSwapInterval(1);
    window = glfwCreateWindow(display_w, display_h, "ForgeTool", nullptr, nullptr);

    IMGUI_CHECKVERSION();
    gui_ctx = ImGui::CreateContext();
    plt_ctx = ImPlot::CreateContext();
    ImGuiStyle &style = ImGui::GetStyle();
    ApplyGeometry();
    ApplyDarkTheme();
    ApplyFonts();
    ImGui::PushFont(fonts.N);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void ForgeTool::onDeinit()
{
    // Cleanup
    // Destroy child windows //TODO
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void ForgeTool::run()
{
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        // Main loop
        glfwMakeContextCurrent(window);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        glfwGetFramebufferSize(window, &display_w, &display_h);
        render();
        for(auto &r: launchables)
        {
            glfwMakeContextCurrent(r->window);
            r->render();
        }
        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
}

void ForgeTool::render()
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