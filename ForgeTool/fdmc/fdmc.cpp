#include "fdmc.h"
#include "Abf.h"
void LaunchFdmc::FrameHdl(uint8_t id, std::span<uint8_t> data) {
  switch (id) {
  case AbfId_Rpc: rpctool.Process(data); break;
  default: break;
  }
}
void LaunchFdmc::RenderImpl() {
  const float spacing  = ImGui::GetStyle().ItemSpacing.x;
  const auto  avail    = ImGui::GetContentRegionAvail();
  const float minWidth = 300.0f;
  rpctool.Update();
  size_t length = comm ? comm->Read(rx_buf) : 0;
  if (length) {
    size_t offset = 0;
    while (offset < length) {
      AbfReadEvent e  = Abf_Read(&frame_reader, rx_buf.data() + offset, length - offset, &frame);
      offset         += frame.pos;
      if (e == AbfReadEvent_Frame) {
        AbfHdr h = Abf_HdrDecode(frame.hdr);
        FrameHdl(h.id, std::span<uint8_t>(frame.bBuf, h.sz));
      } else if (e == AbfReadEvent_Err) {
      }
    }
  }
  const auto renderRequest = [this](ImVec2 size) {
    if (rpctool.RenderRpcEx(size)) {
      uint8_t abf_header[ABF_HDR_SZ];
      if (rpctool.Call(tx_buf)) {
        if (!comm) return;
        if (Abf_Write(abf_header, tx_buf.data(), tx_buf.size(), AbfId_Rpc) == AbfErr_Ok) {
          comm->Write(abf_header);
          comm->Write(tx_buf);
        }
      }
    }
  };
  if (avail.x >= 2.0f * minWidth + spacing) {
    // Horizontal
    const float width = (avail.x - spacing) * 0.5f;
    renderRequest(ImVec2(width, 0));
    ImGui::SameLine();
    rpctool.RenderRpcRet(ImVec2(width, 0));
  } else {
    // Vertical
    renderRequest(ImVec2(0, avail.y * 0.5f));
    ImGui::Separator();
    rpctool.RenderRpcRet(ImVec2(0, 0));
  }
}
