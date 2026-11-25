#include "fdmc.h"

void LaunchFdmc::frameHandle(uint8_t id, std::span<uint8_t> data) {
  switch (id) {
  case AbfId_RPC: rpctool.Process(data); break;

  default: break;
  }
}

void LaunchFdmc::renderImpl() {
  const float spacing  = ImGui::GetStyle().ItemSpacing.x;
  const auto  avail    = ImGui::GetContentRegionAvail();
  const float minWidth = 300.0f;
  rpctool.update();
  size_t length = comm ? comm->read(rx_buf) : 0;
  if (length) {
    size_t offset = 0;
    while (offset < length) {
      AbfReadEvent e  = abf_read(&frame_reader, rx_buf.data() + offset, length - offset, &frame);
      offset         += frame.pos;
      if (e == AbfReadEvent_FRAME) {
        AbfHeader_t h = abf_header_decode(frame.header);
        frameHandle(h.id, std::span<uint8_t>(frame.b_buf, h.size));
      } else if (e == AbfReadEvent_ERROR) {
      }
    }
  }

  const auto renderRequest = [this](ImVec2 size) {
    if (rpctool.renderRpcEx(size)) {
      uint8_t abf_header[ABF_HEADER_SIZE];
      if (rpctool.Call(tx_buf)) {
        if (!comm) return;
        if (abf_write(abf_header, tx_buf.data(), tx_buf.size(), AbfId_RPC) == AbfErr_OK) {
          comm->write(abf_header);
          comm->write(tx_buf);
        }
      }
    }
  };

  if (avail.x >= 2.0f * minWidth + spacing) {
    // Horizontal
    const float width = (avail.x - spacing) * 0.5f;
    renderRequest(ImVec2(width, 0));
    ImGui::SameLine();
    rpctool.renderRpcRet(ImVec2(width, 0));
  } else {
    // Vertical
    renderRequest(ImVec2(0, avail.y * 0.5f));
    ImGui::Separator();
    rpctool.renderRpcRet(ImVec2(0, 0));
  }
}
