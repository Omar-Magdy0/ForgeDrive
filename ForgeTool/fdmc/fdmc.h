#include "ForgeTool.h"
#include "fdrpcTool.h"
#include <iostream>
extern "C" {
#include "AbfStream.h"
}
class LaunchFdmc : public Launchable {
  FdrpcTool            rpctool;
  IComm               *comm;
  AbfReader_t          frame_reader;
  AbfRead_t            frame;
  std::vector<uint8_t> tx_buf;
  std::vector<uint8_t> b_buf;
  std::vector<uint8_t> rx_buf;

public:
  LaunchFdmc(ForgeTool &fg, std::string name)
      : Launchable(fg, "ForgeDriveMC", name), rpctool(fg,
                                                      {"/media/renderer/New Volume/Shared_data/ForgeDrive/ForgeDriveMC/fdmc",
                                                       "/media/renderer/New Volume/Shared_data/ForgeDrive/fdlib/middleware/fdrpc", "/usr/include"},
                                                      "fdmc.proto") {
    tx_buf.resize(255);
    b_buf.resize(255);
    rx_buf.resize(1 << 15);
    frame.b_buf    = b_buf.data();
    frame.b_buf_sz = b_buf.capacity();
    abf_init_reader(&frame_reader);
  }
  void frameHandle(uint8_t id, std::span<uint8_t> data);
  void setComm(IComm *comm_) {
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