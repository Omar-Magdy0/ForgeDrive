#include "App.h"
#include <vector>
#include <math.h>
#include <iostream>


void App::onFrame(void *ctx, uint8_t id, uint8_t *payload, uint8_t payload_len)
{
}

void App::onError(void *ctx, uint8_t id)
{
}

void App::run()
{
    gui_init();

    while(1)gui_loop();
}


//======================================================================
//  GLFW/IMGUI Boilerplate
//======================================================================



#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

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

void App::gui_init(){
    // Initialize GLFW
    // Setup error callback
    // Create window
    
    glfwSetErrorCallback(glfw_error_callback);
    
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
    }
    // Setup OpenGL version
#if defined(__APPLE__)
    // GL 3.2 + GLSL 150 (MacOS)
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on MacOS
#else
    // GL 3.0 + GLSL 130 (Windows and Linux)
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    window = glfwCreateWindow(display_w, display_h, "VIRTUAL_PMSM", nullptr, nullptr);
    glfwSetWindowCloseCallback(window, window_close_callback);

    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(3);

    // Setup context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
	ImGuiIO& io = ImGui::GetIO();


    // Setup style
    ImGui::StyleColorsDark();

    // Setup backend
    ImGui::StyleColorsDark(); // Or ImGui::StyleColorsLight();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImFont* font = nullptr;
    #ifdef _WIN32
        // Windows standard font path
        font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    #else
        auto try_load_font = [&](const char* path) -> ImFont* {
            FILE* file = fopen(path, "rb");
            if (!file) return nullptr;
            fclose(file);
            return io.Fonts->AddFontFromFileTTF(path, 18.0f);
        };

        font = try_load_font("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf");
        if (!font) font = try_load_font("/usr/share/fonts/TTF/DejaVuSans.ttf");
        if (!font) font = try_load_font("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    #endif

    // Fallback if the specific system fonts above aren't found
    if (font == nullptr) {
        fprintf(stderr, "System font not found, using default Dear ImGui font.\n");
    }

    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark(); // Dark theme is the modern standard

    // --- Soften the UI ---
    style.WindowRounding    = 6.0f;  // Round window corners
    style.FrameRounding     = 4.0f;  // Round buttons/inputs
    style.ScrollbarRounding = 9.0f;  // Round scrollbars
    style.GrabRounding      = 4.0f;  // Round sliders
    style.PopupRounding     = 4.0f;  // Round right-click menus
    
    // --- Clean up Colors ---
    // Make the background a bit darker and cleaner
    style.Colors[ImGuiCol_WindowBg]         = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_Header]           = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered]    = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    
    // --- Anti-aliasing (Crucial for ImPlot) ---
    style.AntiAliasedLines = true;
    style.AntiAliasedFill  = true;
    
    //ADD MY GUI INIT
}
void App::gui_deinit()
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}
void App::gui_loop() {
    if(!glfwWindowShouldClose(window)) {
        // Main loop
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glfwPollEvents();
        
        gui.render();

        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
}
