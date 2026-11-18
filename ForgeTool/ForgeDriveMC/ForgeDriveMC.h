#include "ForgeTool.h"
#include "RpcPage.h"

class LaunchForgeDriveMC : public Launchable
{
    RpcPage rpcPage;
    public:
    LaunchForgeDriveMC(ForgeTool& fg, std::string name): Launchable(fg, "ForgeDriveMC", name), rpcPage(fg){}
    void renderImpl() override;
};