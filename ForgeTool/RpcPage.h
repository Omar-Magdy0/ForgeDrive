#pragma once
#include "ForgeTool.h"
#include <iterator>
class RpcPage
{
    private:
    ForgeTool &fg;
    int selectedRpc = -1;
    public:
    RpcPage(ForgeTool &fg_) : fg(fg_)
    {
    }
    void render(ImVec2 dim)
    {
        if (ImGui::BeginChild("RpcPage", dim))
        {
            
    
            // Render Input fields based on the Rpc selection

        }
        ImGui::EndChild();
    }
};
