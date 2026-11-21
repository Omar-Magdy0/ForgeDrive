#include "ForgeTool.h"
#include "RpcPage.h"

class LaunchFdmc : public Launchable
{
    RpcPage rpcPage;
    public:
    LaunchFdmc(ForgeTool& fg, std::string name): Launchable(fg, "ForgeDriveMC", name), rpcPage(fg){}
    void renderImpl() override;
};