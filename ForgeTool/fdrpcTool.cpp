#include "fdrpcTool.h"
#include <algorithm>
#include <format>
const FdrpcTool::Method* FdrpcTool::methodSelection() {
  if (fdesc->service_count() == 0) return nullptr;
  const auto* service = fdesc->service(selected_service_);
  if (ImGui::BeginCombo("SERVICE", service->name().c_str())) {
    for (int i = 0; i < fdesc->service_count(); ++i) {
      const auto* item = fdesc->service(i);
      if (ImGui::Selectable(item->name().c_str(), selected_service_ == i)) {
        selected_service_ = i;
        selected_method_  = 0;
      }
      if (selected_service_ == i) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  service = fdesc->service(selected_service_);
  if (service->method_count() == 0) return nullptr;
  if (selected_method_ >= service->method_count()) selected_method_ = 0;
  const auto* method      = service->method(selected_method_);
  auto        methodLabel = [&](const auto* method) { return method->name() + " (ID: " + std::to_string(getRpcId(method)) + ")"; };
  // RPC selection
  if (ImGui::BeginCombo("RPC", methodLabel(method).c_str())) {
    for (int i = 0; i < service->method_count(); ++i) {
      const auto* item = service->method(i);
      if (ImGui::Selectable(methodLabel(item).c_str(), selected_method_ == i)) selected_method_ = i;
      if (selected_method_ == i) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  return service->method(selected_method_);
}
void FdrpcTool::DrawFieldInputRepeated(Message* msg, const Field* field) {
  const auto* refl  = msg->GetReflection();
  const int   count = refl->FieldSize(*msg, field);
  ImGui::PushID(field->name().c_str());
  const std::string indexedName = field->name() + " [";
  for (int i = 0; i < count; ++i) {
    ImGui::PushID(i);
    const std::string name   = indexedName + std::to_string(i) + "]";
    bool              remove = ImGui::SmallButton("-");
    ImGui::SameLine();
    switch (field->cpp_type()) {
    case Field::CPPTYPE_INT32: {
      auto v = refl->GetRepeatedInt32(*msg, field, i);
      if (ImGui::InputInt(name.c_str(), &v)) refl->SetRepeatedInt32(msg, field, i, v);
      break;
    }
    case Field::CPPTYPE_INT64: {
      auto v = refl->GetRepeatedInt64(*msg, field, i);
      if (ImGui::InputScalar(name.c_str(), ImGuiDataType_S64, &v)) refl->SetRepeatedInt64(msg, field, i, v);
      break;
    }
    case Field::CPPTYPE_UINT32: {
      auto v = refl->GetRepeatedUInt32(*msg, field, i);
      if (ImGui::InputScalar(name.c_str(), ImGuiDataType_U32, &v)) refl->SetRepeatedUInt32(msg, field, i, v);
      break;
    }
    case Field::CPPTYPE_UINT64: {
      auto v = refl->GetRepeatedUInt64(*msg, field, i);
      if (ImGui::InputScalar(name.c_str(), ImGuiDataType_U64, &v)) refl->SetRepeatedUInt64(msg, field, i, v);
      break;
    }
    case Field::CPPTYPE_DOUBLE: {
      auto v = refl->GetRepeatedDouble(*msg, field, i);
      if (ImGui::InputDouble(name.c_str(), &v, 0, 0, "%.17g")) refl->SetRepeatedDouble(msg, field, i, v);
      break;
    }
    case Field::CPPTYPE_FLOAT: {
      auto v = refl->GetRepeatedFloat(*msg, field, i);
      if (ImGui::InputFloat(name.c_str(), &v, 0, 0, "%.9g")) refl->SetRepeatedFloat(msg, field, i, v);
      break;
    }
    case Field::CPPTYPE_BOOL: {
      auto v = refl->GetRepeatedBool(*msg, field, i);
      if (ImGui::Checkbox(name.c_str(), &v)) refl->SetRepeatedBool(msg, field, i, v);
      break;
    }
    case Field::CPPTYPE_ENUM: {
      const auto* enum_desc = field->enum_type();
      int         value     = refl->GetRepeatedEnumValue(*msg, field, i);
      const auto* current   = enum_desc->FindValueByNumber(value);
      if (ImGui::BeginCombo(name.c_str(), current ? current->name().c_str() : "Unknown")) {
        for (int n = 0; n < enum_desc->value_count(); ++n) {
          const auto* item = enum_desc->value(n);
          if (ImGui::Selectable(item->name().c_str(), item->number() == value)) refl->SetRepeatedEnumValue(msg, field, i, item->number());
        }
        ImGui::EndCombo();
      }
      break;
    }
    case Field::CPPTYPE_STRING: {
      std::string value = refl->GetRepeatedString(*msg, field, i);
      char        buffer[1024]{};
      std::copy_n(value.data(), std::min(value.size(), sizeof(buffer) - 1), buffer);
      if (ImGui::InputText(name.c_str(), buffer, sizeof(buffer))) refl->SetRepeatedString(msg, field, i, buffer);
      break;
    }
    case Field::CPPTYPE_MESSAGE: DrawMessageInput(refl->MutableRepeatedMessage(msg, field, i), name); break;
    }
    ImGui::PopID();
    if (remove) {
      refl->SwapElements(msg, field, i, count - 1);
      refl->RemoveLast(msg, field);
      break;
    }
  }
  if (ImGui::SmallButton("+")) {
    switch (field->cpp_type()) {
    case Field::CPPTYPE_INT32: refl->AddInt32(msg, field, 0); break;
    case Field::CPPTYPE_INT64: refl->AddInt64(msg, field, 0); break;
    case Field::CPPTYPE_UINT32: refl->AddUInt32(msg, field, 0); break;
    case Field::CPPTYPE_UINT64: refl->AddUInt64(msg, field, 0); break;
    case Field::CPPTYPE_DOUBLE: refl->AddDouble(msg, field, 0); break;
    case Field::CPPTYPE_FLOAT: refl->AddFloat(msg, field, 0); break;
    case Field::CPPTYPE_BOOL: refl->AddBool(msg, field, false); break;
    case Field::CPPTYPE_ENUM: refl->AddEnum(msg, field, field->default_value_enum()); break;
    case Field::CPPTYPE_STRING: refl->AddString(msg, field, ""); break;
    case Field::CPPTYPE_MESSAGE: refl->AddMessage(msg, field); break;
    }
  }
  ImGui::PopID();
}
bool FdrpcTool::DrawOneofInput(Message* msg, const Field* field) {
  const auto* oneof = field->containing_oneof();
  if (!oneof) return true;
  const auto* refl   = msg->GetReflection();
  const auto* active = refl->GetOneofFieldDescriptor(*msg, oneof);
  if (field == oneof->field(0)) {
    const char* preview = active ? active->name().c_str() : "None";
    if (ImGui::BeginCombo(oneof->name().c_str(), preview)) {
      ImGui::PushID(oneof->name().c_str());
      for (int i = 0; i < oneof->field_count(); ++i) {
        const auto* item = oneof->field(i);
        if (ImGui::Selectable(item->name().c_str(), active == item)) {
          refl->ClearOneof(msg, oneof);
          switch (item->cpp_type()) {
          case Field::CPPTYPE_INT32: refl->SetInt32(msg, item, item->default_value_int32()); break;
          case Field::CPPTYPE_INT64: refl->SetInt64(msg, item, item->default_value_int64()); break;
          case Field::CPPTYPE_UINT32: refl->SetUInt32(msg, item, item->default_value_uint32()); break;
          case Field::CPPTYPE_UINT64: refl->SetUInt64(msg, item, item->default_value_uint64()); break;
          case Field::CPPTYPE_DOUBLE: refl->SetDouble(msg, item, item->default_value_double()); break;
          case Field::CPPTYPE_FLOAT: refl->SetFloat(msg, item, item->default_value_float()); break;
          case Field::CPPTYPE_BOOL: refl->SetBool(msg, item, item->default_value_bool()); break;
          case Field::CPPTYPE_ENUM: refl->SetEnum(msg, item, item->default_value_enum()); break;
          case Field::CPPTYPE_STRING: refl->SetString(msg, item, item->default_value_string()); break;
          case Field::CPPTYPE_MESSAGE: refl->MutableMessage(msg, item); break;
          }
          active = item;
        }
        if (active == item) ImGui::SetItemDefaultFocus();
      }
      ImGui::PopID();
      ImGui::EndCombo();
    }
  }
  return active == field;
}
void FdrpcTool::DrawFieldInput(Message* msg, const Field* field) {
  const char* name = field->name().c_str();
  const auto* refl = msg->GetReflection();
  if (field->is_map()) {
    // Protobuf maps are represented by repeated entry messages. Reuse the
    // repeated-field editor so each entry exposes editable key/value fields
    // and can be added or removed with the existing controls.
    DrawFieldInputRepeated(msg, field);
    return;
  }
  if (!DrawOneofInput(msg, field)) return;
  if (field->is_repeated()) {
    DrawFieldInputRepeated(msg, field);
    return;
  }
  switch (field->cpp_type()) {
  case Field::CPPTYPE_INT32: {
    auto v = refl->GetInt32(*msg, field);
    if (ImGui::InputInt(name, &v)) refl->SetInt32(msg, field, v);
    break;
  }
  case Field::CPPTYPE_INT64: {
    auto v = refl->GetInt64(*msg, field);
    if (ImGui::InputScalar(name, ImGuiDataType_S64, &v)) refl->SetInt64(msg, field, v);
    break;
  }
  case Field::CPPTYPE_UINT32: {
    auto v = refl->GetUInt32(*msg, field);
    if (ImGui::InputScalar(name, ImGuiDataType_U32, &v)) refl->SetUInt32(msg, field, v);
    break;
  }
  case Field::CPPTYPE_UINT64: {
    auto v = refl->GetUInt64(*msg, field);
    if (ImGui::InputScalar(name, ImGuiDataType_U64, &v)) refl->SetUInt64(msg, field, v);
    break;
  }
  case Field::CPPTYPE_DOUBLE: {
    auto v = refl->GetDouble(*msg, field);
    if (ImGui::InputDouble(name, &v, 0, 0, "%.17g")) refl->SetDouble(msg, field, v);
    break;
  }
  case Field::CPPTYPE_FLOAT: {
    auto v = refl->GetFloat(*msg, field);
    if (ImGui::InputFloat(name, &v, 0, 0, "%.9g")) refl->SetFloat(msg, field, v);
    break;
  }
  case Field::CPPTYPE_BOOL: {
    auto v = refl->GetBool(*msg, field);
    if (ImGui::Checkbox(name, &v)) refl->SetBool(msg, field, v);
    break;
  }
  case Field::CPPTYPE_ENUM: {
    const auto* enum_desc = field->enum_type();
    int         v         = refl->GetEnumValue(*msg, field);
    auto        prev      = enum_desc->FindValueByNumber(v) ? enum_desc->FindValueByNumber(v)->name().c_str() : "Unknown";
    if (ImGui::BeginCombo(name, prev)) {
      ImGui::PushID(name);
      for (int i = 0; i < enum_desc->value_count(); ++i) {
        const auto* item     = enum_desc->value(i);
        bool        selected = item->number() == v;
        if (ImGui::Selectable(item->name().c_str(), selected)) refl->SetEnumValue(msg, field, item->number());
        if (selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::PopID();
      ImGui::EndCombo();
    }
    break;
  }
  case Field::CPPTYPE_STRING: {
    std::string value = refl->GetString(*msg, field);
    char        buffer[1024]{};
    std::copy_n(value.data(), std::min(value.size(), sizeof(buffer) - 1), buffer);
    if (ImGui::InputText(name, buffer, sizeof(buffer))) refl->SetString(msg, field, buffer);
    break;
  }
  case Field::CPPTYPE_MESSAGE: DrawMessageInput(refl->MutableMessage(msg, field), name); break;
  }
}
void FdrpcTool::DrawMessageInput(Message* msg, const std::string& name) {
  const auto*       desc     = msg->GetDescriptor();
  const std::string nodeName = desc->name() + (name.empty() ? "" : " " + name);
  const std::string fullName = desc->full_name() + nodeName;
  if (!ImGui::TreeNodeEx(nodeName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) return;
  ImGui::PushID(fullName.c_str());
  for (int i = 0; i < desc->field_count(); ++i) DrawFieldInput(msg, desc->field(i));
  ImGui::TreePop();
  ImGui::PopID();
}
void FdrpcTool::DrawMessageOutput(Message* msg, const std::string& name) {
  ImGui::BeginDisabled();
  DrawMessageInput(msg, name);
  ImGui::EndDisabled();
}
bool FdrpcTool::renderRpcEx(ImVec2 dim) {
  if (!ImGui::BeginChild("RpcEx", dim)) {
    ImGui::EndChild();
    return false;
  }
  if (!fdesc) {
    ImGui::InputTextMultiline("##diagnostics", err.data(), err.size(), ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_WordWrap);
    ImGui::EndChild();
    return false;
  }
  bool send = false;
  ImGui::BeginDisabled(!call_handle.complete);
  if (auto* desc = methodSelection()) {
    send = ImGui::Button("Send");
    ImGui::SameLine();
    ImGui::Text("REQ:%u", call_handle.rid);
    ImGui::Separator();
    if (desc != call_handle.desc) {
      const bool chain = call_handle.desc && call_handle.ret && (call_handle.desc->output_type() == desc->input_type());
      call_handle.desc = desc;
      if (chain) call_handle.arg.reset(call_handle.ret.release());
      else call_handle.arg.reset(factory.GetPrototype(call_handle.desc->input_type())->New());
      call_handle.ret.reset(factory.GetPrototype(call_handle.desc->output_type())->New());
      call_handle.desc = desc;
    }
    DrawMessageInput(call_handle.arg.get());
  }
  ImGui::EndDisabled();
  ImGui::EndChild();
  return send;
}
bool FdrpcTool::renderRpcRet(ImVec2 dim) {
  ImGui::BeginGroup();
  const float errorHeight  = 80.0f;
  const float outputHeight = ImGui::GetContentRegionAvail().y - errorHeight - ImGui::GetStyle().ItemSpacing.y;
  ImGui::BeginChild("RpcRetOutput", ImVec2(0, outputHeight), ImGuiChildFlags_None);
  if (fdesc) { DrawMessageOutput(call_handle.ret.get()); }
  ImGui::EndChild();
  ImGui::PushFont(fg.Style().fonts.S);
  if (ImGui::SmallButton("Clear")) dispatch_err.clear();
  DrawError(ImVec2(0, 0));
  ImGui::PopFont();
  ImGui::EndGroup();
  return false;
}
uint32_t FdrpcTool::getRpcId(const Method* method) { return method->options().GetExtension(fdrpc::id); }
bool     FdrpcTool::deserialize(Message* msg, const std::vector<uint8_t>& data) {
  if (data.size() > INT_MAX) return false;
  return msg->ParseFromArray(data.data(), static_cast<int>(data.size()));
}
void FdrpcTool::update() {
  if (!call_handle.complete && Clock::now() >= call_handle.deadline) {
    call_handle.complete = true;
    dispatch_err.emplace_back(std::format("RPC Timeout |ID:{}|REQ:{}", getRpcId(call_handle.desc), call_handle.rid));
  }
}
bool FdrpcTool::Call(std::vector<uint8_t>& data) {
  const uint16_t id = getRpcId(call_handle.desc);
  if (!call_handle.complete) return false;
  call_handle.rid++;
  data.resize(4 + call_handle.arg->ByteSizeLong());
  data[0] = static_cast<uint8_t>(id);
  data[1] = static_cast<uint8_t>(id >> 8);
  data[2] = static_cast<uint8_t>(call_handle.rid);
  data[3] = static_cast<uint8_t>(call_handle.rid >> 8);
  if (!call_handle.arg->SerializeToArray(data.data() + 4, static_cast<int>(data.size() - 4))) {
    data.clear();
    return false;
  }
  call_handle.deadline = Clock::now() + timeout;
  call_handle.desc     = call_handle.desc;
  call_handle.complete = false;
  return true;
}
Rpc::RpcType FdrpcTool::Process(std::span<uint8_t> data) {
  if (data.size() < 4) return Rpc::RpcType_NONE;
  const uint16_t id = static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
  Rpc::RpcType   tp = Rpc::idRpcType(id);
  switch (tp) {
  case Rpc::RpcType::RpcType_ERROR: {
    if (data.size() < 7) return Rpc::RpcType_NONE;
    err_handle.id  = static_cast<uint16_t>(data[2]) | (static_cast<uint16_t>(data[3]) << 8);
    err_handle.rid = static_cast<uint16_t>(data[4]) | (static_cast<uint16_t>(data[5]) << 8);
    err_handle.err = static_cast<Rpc::DispatchStatus>(data[6]);
    dispatch_err.emplace_back(std::format("Dispatch Error |ID:{}|REQ:{}|{}", err_handle.id, err_handle.rid, Rpc::dispatchStatusToString(err_handle.err)));
    return Rpc::RpcType_ERROR;
  }
  case Rpc::RpcType::RpcType_CALL: {
    const uint16_t rid = static_cast<uint16_t>(data[2]) | (static_cast<uint16_t>(data[3]) << 8);
    if (id != getRpcId(call_handle.desc) || rid != call_handle.rid) // only one ongoing call and associated return associated
      return Rpc::RpcType_NONE;
    if (!call_handle.ret->ParseFromArray(data.data() + 4, static_cast<int>(data.size() - 4))) { return Rpc::RpcType_NONE; }
    call_handle.complete = true;
    return Rpc::RpcType_CALL;
  }
  }
  return Rpc::RpcType_NONE;
}
void FdrpcTool::DrawError(ImVec2 dim) {
  ImGui::BeginChild("DispatchErrors", dim, ImGuiChildFlags_Borders);
  ImGui::PushStyleColor(ImGuiCol_Text, fg.Style().colors.Error);
  for (const auto& err : dispatch_err) ImGui::TextWrapped("%s", err.c_str());
  ImGui::PopStyleColor();
  ImGui::EndChild();
}