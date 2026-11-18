#pragma once
#include "ForgeTool.h"
#include "RpcSchema.h"
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
            uint16_t id;
            const char *preview = "Select RPC";
            if (selectedRpc >= 0)
                preview = ForgeDriveMC::rpcInfo[selectedRpc].name;

            if (ImGui::BeginCombo("RPC Method", preview))
            {
                for (int i = 0; i < std::size(ForgeDriveMC::rpcInfo); i++)
                {
                    const auto &rpc = ForgeDriveMC::rpcInfo[i];
                    id = rpc.id;
                    char label[64];
                    snprintf(label, sizeof(label), "%u : %s", rpc.id, rpc.name);
                    bool selected = (selectedRpc == i);
                    if (ImGui::Selectable(label, selected))
                        selectedRpc = i;
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            // Render Input fields based on the Rpc selection

        }
        ImGui::EndChild();
    }
};
