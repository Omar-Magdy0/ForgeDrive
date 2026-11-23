#include "fdrpcTool.h"
#include <algorithm>

const FdrpcTool::Method *FdrpcTool::methodSelection()
{
    if (desc->service_count() == 0)
        return nullptr;
    const auto *service = desc->service(selected_service_);
    if (ImGui::BeginCombo("SERVICE", service->name().c_str()))
    {
        for (int i = 0; i < desc->service_count(); ++i)
        {
            const auto *item = desc->service(i);
            if (ImGui::Selectable(item->name().c_str(), selected_service_ == i))
            {
                selected_service_ = i;
                selected_method_ = 0;
            }
            if (selected_service_ == i)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    service = desc->service(selected_service_);
    if (service->method_count() == 0)
        return nullptr;
    if (selected_method_ >= service->method_count())
        selected_method_ = 0;
    const auto *method = service->method(selected_method_);
    auto getRpcId = [&](const Method *method) -> uint32_t
    {
        return method->options().GetExtension(fdrpc::id);
    };
    auto methodLabel = [&](const auto *method)
    {
        return method->name() + " (ID: " + std::to_string(getRpcId(method)) + ")";
    };
    // RPC selection
    if (ImGui::BeginCombo("RPC", methodLabel(method).c_str()))
    {
        for (int i = 0; i < service->method_count(); ++i)
        {
            const auto *item = service->method(i);
            if (ImGui::Selectable(methodLabel(item).c_str(), selected_method_ == i))
                selected_method_ = i;
            if (selected_method_ == i)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return service->method(selected_method_);
}

void FdrpcTool::DrawFieldInputRepeated(Message *msg, const Field *field)
{
    const auto *refl = msg->GetReflection();
    const int count = refl->FieldSize(*msg, field);
    ImGui::PushID(field->name().c_str());
    const std::string indexedName = field->name() + " [";
    for (int i = 0; i < count; ++i)
    {
        ImGui::PushID(i);
        const std::string name = indexedName + std::to_string(i) + "]";
        bool remove = ImGui::SmallButton("-");
        ImGui::SameLine();
        switch (field->cpp_type())
        {
        case Field::CPPTYPE_INT32: { auto v = refl->GetRepeatedInt32(*msg, field, i); if (ImGui::InputInt(name.c_str(), &v)) refl->SetRepeatedInt32(msg, field, i, v); break; }
        case Field::CPPTYPE_INT64: { auto v = refl->GetRepeatedInt64(*msg, field, i); if (ImGui::InputScalar(name.c_str(), ImGuiDataType_S64, &v)) refl->SetRepeatedInt64(msg, field, i, v); break; }
        case Field::CPPTYPE_UINT32: { auto v = refl->GetRepeatedUInt32(*msg, field, i); if (ImGui::InputScalar(name.c_str(), ImGuiDataType_U32, &v)) refl->SetRepeatedUInt32(msg, field, i, v); break; }
        case Field::CPPTYPE_UINT64: { auto v = refl->GetRepeatedUInt64(*msg, field, i); if (ImGui::InputScalar(name.c_str(), ImGuiDataType_U64, &v)) refl->SetRepeatedUInt64(msg, field, i, v); break; }
        case Field::CPPTYPE_DOUBLE: { auto v = refl->GetRepeatedDouble(*msg, field, i); if (ImGui::InputDouble(name.c_str(), &v)) refl->SetRepeatedDouble(msg, field, i, v); break; }
        case Field::CPPTYPE_FLOAT: { auto v = refl->GetRepeatedFloat(*msg, field, i); if (ImGui::InputFloat(name.c_str(), &v)) refl->SetRepeatedFloat(msg, field, i, v); break; }
        case Field::CPPTYPE_BOOL: { auto v = refl->GetRepeatedBool(*msg, field, i); if (ImGui::Checkbox(name.c_str(), &v)) refl->SetRepeatedBool(msg, field, i, v); break; }
        case Field::CPPTYPE_ENUM:
        {
            const auto *enum_desc = field->enum_type();
            int value = refl->GetRepeatedEnumValue(*msg, field, i);
            const auto *current = enum_desc->FindValueByNumber(value);
            if (ImGui::BeginCombo(name.c_str(), current ? current->name().c_str() : "Unknown"))
            {
                for (int n = 0; n < enum_desc->value_count(); ++n)
                {
                    const auto *item = enum_desc->value(n);
                    if (ImGui::Selectable(item->name().c_str(), item->number() == value))
                        refl->SetRepeatedEnumValue(msg, field, i, item->number());
                }
                ImGui::EndCombo();
            }
            break;
        }
        case Field::CPPTYPE_STRING:
        {
            std::string value = refl->GetRepeatedString(*msg, field, i);
            char buffer[1024]{};
            std::copy_n(value.data(), std::min(value.size(), sizeof(buffer) - 1), buffer);
            if (ImGui::InputText(name.c_str(), buffer, sizeof(buffer)))
                refl->SetRepeatedString(msg, field, i, buffer);
            break;
        }
        case Field::CPPTYPE_MESSAGE:
            DrawMessageInput(refl->MutableRepeatedMessage(msg, field, i), name);
            break;
        }
        ImGui::PopID();
        if (remove)
        {
            refl->SwapElements(msg, field, i, count - 1);
            refl->RemoveLast(msg, field);
            break;
        }
    }
    if (ImGui::SmallButton("+"))
    {
        switch (field->cpp_type())
        {
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

bool FdrpcTool::DrawOneofInput(Message *msg, const Field *field)
{
    const auto *oneof = field->containing_oneof();
    if (!oneof)
        return true;

    const auto *refl = msg->GetReflection();
    const auto *active = refl->GetOneofFieldDescriptor(*msg, oneof);
    if (field == oneof->field(0))
    {
        const char *preview = active ? active->name().c_str() : "None";
        if (ImGui::BeginCombo(oneof->name().c_str(), preview))
        {
            ImGui::PushID(oneof->name().c_str());
            for (int i = 0; i < oneof->field_count(); ++i)
            {
                const auto *item = oneof->field(i);
                if (ImGui::Selectable(item->name().c_str(), active == item))
                {
                    refl->ClearOneof(msg, oneof);
                    switch (item->cpp_type())
                    {
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
                if (active == item)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::PopID();
            ImGui::EndCombo();
        }
    }
    return active == field;
}

void FdrpcTool::DrawFieldInput(Message *msg, const Field *field)
{
    const char *name = field->name().c_str();
    const auto *refl = msg->GetReflection();
    if (field->is_map())
    {
        // Protobuf maps are represented by repeated entry messages. Reuse the
        // repeated-field editor so each entry exposes editable key/value fields
        // and can be added or removed with the existing controls.
        DrawFieldInputRepeated(msg, field);
        return;
    }
    if (!DrawOneofInput(msg, field)) return;
    if (field->is_repeated()) { DrawFieldInputRepeated(msg, field); return; }
    switch (field->cpp_type())
    {
    case Field::CPPTYPE_INT32: { auto v = refl->GetInt32(*msg, field); if (ImGui::InputInt(name, &v)) refl->SetInt32(msg, field, v); break; }
    case Field::CPPTYPE_INT64: { auto v = refl->GetInt64(*msg, field); if (ImGui::InputScalar(name, ImGuiDataType_S64, &v)) refl->SetInt64(msg, field, v); break; }
    case Field::CPPTYPE_UINT32: { auto v = refl->GetUInt32(*msg, field); if (ImGui::InputScalar(name, ImGuiDataType_U32, &v)) refl->SetUInt32(msg, field, v); break; }
    case Field::CPPTYPE_UINT64: { auto v = refl->GetUInt64(*msg, field); if (ImGui::InputScalar(name, ImGuiDataType_U64, &v)) refl->SetUInt64(msg, field, v); break; }
    case Field::CPPTYPE_DOUBLE: { auto v = refl->GetDouble(*msg, field); if (ImGui::InputDouble(name, &v)) refl->SetDouble(msg, field, v); break; }
    case Field::CPPTYPE_FLOAT: { auto v = refl->GetFloat(*msg, field); if (ImGui::InputFloat(name, &v)) refl->SetFloat(msg, field, v); break; }
    case Field::CPPTYPE_BOOL: { auto v = refl->GetBool(*msg, field); if (ImGui::Checkbox(name, &v)) refl->SetBool(msg, field, v); break; }
    case Field::CPPTYPE_ENUM:
    {
        const auto *enum_desc = field->enum_type();
        int v = refl->GetEnumValue(*msg, field);
        auto prev = enum_desc->FindValueByNumber(v) ? enum_desc->FindValueByNumber(v)->name().c_str() : "Unknown";
        if (ImGui::BeginCombo(name, prev))
        {
            ImGui::PushID(name);
            for (int i = 0; i < enum_desc->value_count(); ++i)
            {
                const auto *item = enum_desc->value(i);
                bool selected = item->number() == v;
                if (ImGui::Selectable(item->name().c_str(), selected))
                    refl->SetEnumValue(msg, field, item->number());
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::PopID();
            ImGui::EndCombo();
        }
        break;
    }
    case Field::CPPTYPE_STRING: { std::string value = refl->GetString(*msg, field); char buffer[1024]{}; std::copy_n(value.data(), std::min(value.size(), sizeof(buffer) - 1), buffer); if (ImGui::InputText(name, buffer, sizeof(buffer))) refl->SetString(msg, field, buffer); break; }
    case Field::CPPTYPE_MESSAGE: DrawMessageInput(refl->MutableMessage(msg, field), name); break;
    }
}

void FdrpcTool::DrawMessageInput(Message *msg, const std::string &fieldName)
{
    const auto *desc = msg->GetDescriptor();
    const std::string nodeName = desc->name() + (fieldName.empty() ? "" : " " + fieldName);
    const std::string fullName = desc->full_name() + nodeName;
    if (!ImGui::TreeNodeEx(nodeName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        return;
    ImGui::PushID(fullName.c_str());
    for (int i = 0; i < desc->field_count(); ++i)
        DrawFieldInput(msg, desc->field(i));
    ImGui::TreePop();
    ImGui::PopID();
}

void FdrpcTool::render(ImVec2 dim)
{
    if (!ImGui::BeginChild("FdrpcTool", dim))
    {
        ImGui::EndChild();
        return;
    }
    if (!desc)
    {
        ImGui::InputTextMultiline("##diagnostics", err.data(), err.size(), ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_WordWrap);
        ImGui::EndChild();
        return;
    }
    if (auto *method = methodSelection())
    {
        if (method != method_)
        {
            method_ = method;
            arg.reset(factory.GetPrototype(method_->input_type())->New());
            ret.reset(factory.GetPrototype(method_->output_type())->New());
        }
        DrawMessageInput(arg.get());
    }

    ImGui::EndChild();
}
