#include "FdrpcTool.h"
#include "ForgeTool.h"
#include <iostream>
extern "C" {
#include "Abf.h"
}
class LaunchFdmc : public Launchable {
  FdrpcTool            rpctool;
  IComm*               comm;
  AbfReader            frame_reader;
  AbfRead              frame;
  std::vector<uint8_t> tx_buf;
  std::vector<uint8_t> b_buf;
  std::vector<uint8_t> rx_buf;
public:
  LaunchFdmc(ForgeTool& fg, std::string name)
      : Launchable(fg, "ForgeDriveMC", name), rpctool(fg,
                                                      {"/media/renderer/New Volume/Shared_data/ForgeDrive/ForgeDriveMC/fdmc",
                                                       "/media/renderer/New Volume/Shared_data/ForgeDrive/fdlib/middleware/fdrpc", "/usr/include"},
                                                      "fdmc.proto") {
    tx_buf.resize(255);
    b_buf.resize(255);
    rx_buf.resize(1 << 15);
    frame.bBuf   = b_buf.data();
    frame.bBufSz = b_buf.capacity();
    Abf_InitReader(&frame_reader);
  }
  void frameHandle(uint8_t id, std::span<uint8_t> data);
  void setComm(IComm* comm_) {
    if (!comm_) return;
    comm               = comm_;
    comm->onDisconnect = [this]() {
      this->comm = nullptr;
      this->open = false;
    };
    comm->onError = [this](int) {
      // Handle communication error
    };
  }
  void renderImpl() override;
};