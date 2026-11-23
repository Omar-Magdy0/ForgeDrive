#include "ForgeTool.h"
#include "fdrpcTool.h"

class LaunchFdmc : public Launchable
{
    FdrpcTool rpctool;
    public:
    LaunchFdmc(ForgeTool& fg, std::string name): Launchable(fg, "ForgeDriveMC", name), 
                rpctool(fg, {
                "/media/renderer/New Volume/Shared_data/ForgeDrive/ForgeDriveMC/fdmc",
                "/media/renderer/New Volume/Shared_data/ForgeDrive/fdlib/middleware/fdrpc",
                "/usr/include"
            }, "fdmc.proto"){}
    void renderImpl() override;
};