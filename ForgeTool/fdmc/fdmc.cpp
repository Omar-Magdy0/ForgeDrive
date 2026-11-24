#include "fdmc.h"

void LaunchFdmc::renderImpl()
{
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const auto avail = ImGui::GetContentRegionAvail();
    const float minWidth = 300.0f;
    rpctool.update();
    if (avail.x >= 2.0f * minWidth + spacing)
    {
        // Horizontal
        const float width = (avail.x - spacing) * 0.5f;
        if(rpctool.renderRpcEx(ImVec2(width, 0)))
        {
            std::vector<uint8_t> data;
            rpctool.Call(data);
        }
        ImGui::SameLine();
        rpctool.renderRpcRet(ImVec2(width, 0));
    }
    else
    {
        // Vertical
        if(rpctool.renderRpcEx(ImVec2(0, avail.y*0.5)))
        {            
            std::vector<uint8_t> data;
            rpctool.Call(data);
        }
        ImGui::Separator();
        rpctool.renderRpcRet(ImVec2(0, 0));
    }
}
