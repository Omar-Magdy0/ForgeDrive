#include "Gui.h"
#include <math.h>

void Gui::render()
{
    ImGui::Begin("Telemetry & Control Deck", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    // Pulsing LED Status Indicator
    float time = (float)ImGui::GetTime();
    ImVec4 col_led = ImVec4(0.1f + 0.4f * sinf(time * 5.0f), 0.9f, 0.2f, 1.0f);
    ImGui::Text("Inverter State:");
    ImGui::SameLine();
    ImGui::ColorButton("##led", col_led, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker, ImVec2(16, 16));
    ImGui::SameLine();
    ImGui::TextColored(col_led, "CLOSED-LOOP (FOC)");

    ImGui::Separator();

    // Live Control Sliders
    static float duty = 75.0f;
    static float rpm = 3200.0f;
    ImGui::SliderFloat("PWM Duty", &duty, 0.0f, 100.0f, "%.1f %%");
    ImGui::SliderFloat("Target Speed", &rpm, 0.0f, 6000.0f, "%.0f RPM");

    // Real-time Sparkline Graph
    static float history[90] = {};
    static int offset = 0;
    history[offset] = rpm / 60.0f + (sinf(time * 12.0f) * 25.0f);
    offset = (offset + 1) % IM_ARRAYSIZE(history);
    ImGui::PlotLines("Frequency (Hz)", history, IM_ARRAYSIZE(history), offset, nullptr, 0.0f, 120.0f, ImVec2(0, 70));

    if (ImGui::Button("EMERGENCY TRIP", ImVec2(-1, 32)))
    {
        duty = 0.0f;
    }

    ImGui::End();
}