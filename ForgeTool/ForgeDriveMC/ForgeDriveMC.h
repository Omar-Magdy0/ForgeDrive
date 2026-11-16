#include "ForgeTool.h"


class ForgeDriveMC : public Launchable
{
    public:
    ForgeDriveMC(ForgeTool& fg, std::string name): Launchable(fg, "ForgeDriveMC", name){}
    void renderImpl() override;
};