#include "platform.h"
#include <stdio.h>
#include <stdlib.h>

#include <imgui.h>
#include <implot.h>
#include "silgui.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "IconsFontAwesome7.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <math.h>
#include <iostream>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif


static inline int display_w = 1000;
static inline int display_h = 1000;

GLFWwindow* window;

// Callback to handle GLFW errors
static inline void glfw_error_callback(int error, const char* description) { std::cerr << "GLFW Error " << error << ": " << description << std::endl; }
static inline void window_close_callback(GLFWwindow* w) {
    // Force the window to stay open
    glfwSetWindowShouldClose(w, GLFW_FALSE);
    // You could trigger an ImGui popup here saying "Please use Shutdown button"
}

void imgui_init()
{
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    // Main font
    ImFont* main_font = io.Fonts->AddFontFromFileTTF(
        FONTS_DIR "/DejaVu/DejaVuSans.ttf",
        18.0f
    );

    if (!main_font)
        std::cerr << "Failed to load main font\n";

    // Font Awesome
    ImFontConfig icon_config;
    icon_config.MergeMode = true;
    icon_config.PixelSnapH = true;

    static const ImWchar icon_ranges[] = {
        ICON_MIN_FA,
        ICON_MAX_FA,
        0
    };

    ImFont* icon_font = io.Fonts->AddFontFromFileTTF(
        FONTS_DIR "/FontAwesome7/Font Awesome 7 Free-Solid-900.otf",
        18.0f,
        &icon_config,
        icon_ranges
    );
    
    if (!icon_font)
        std::cerr << "Failed to load Font Awesome\n";

    // Styling
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 4.0f;
    style.PopupRounding = 4.0f;

    style.Colors[ImGuiCol_WindowBg] =
        ImVec4(0.10f, 0.10f, 0.12f, 1.00f);

    style.Colors[ImGuiCol_Header] =
        ImVec4(0.20f, 0.25f, 0.29f, 1.00f);

    style.Colors[ImGuiCol_HeaderHovered] =
        ImVec4(0.26f, 0.59f, 0.98f, 0.80f);

    style.AntiAliasedLines = false;
    style.AntiAliasedLineEnds = true;
    style.AntiAliasedFill = true;
}

void gui_init(){
      glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return;
    }

#if defined(__APPLE__)
    const char* glsl_version = "#version 150";

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char* glsl_version = "#version 130";

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    window = glfwCreateWindow(
        display_w, display_h, "ForgeTool", nullptr, nullptr
    );

    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Create ImGui context and configure resources
    imgui_init();
    silgui_init();

    // Initialize backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void gui_deinit()
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}
void gui_loop() {
    while(!glfwWindowShouldClose(window)) {
        // Main loop
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        glfwGetFramebufferSize(window, &display_w, &display_h);
        silgui_render();
        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    gui_deinit();
}

pthread_t t;
void platform_init()
{
    gui_init();
}