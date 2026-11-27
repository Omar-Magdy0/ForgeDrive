#pragma once
#include "ForgeTool.h"
#include "RpcDef.h"
#include "fdrpc.pb.h"
#include <chrono>
#include <filesystem>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <memory>
#include <vector>
class ErrorCollector : public google::protobuf::compiler::MultiFileErrorCollector {
  struct Diagnostic {
    bool        is_error;
    std::string filename;
    int         line;
    int         column;
    std::string message;
  };
public:
  std::vector<Diagnostic> diagnostics;
  void                    AddError(const std::string& filename, int line, int column, const std::string& message) override {
    diagnostics.push_back({true, filename, line, column, message});
  }
  void AddWarning(const std::string& filename, int line, int column, const std::string& message) override {
    diagnostics.push_back({false, filename, line, column, message});
  }
  std::string toString() const {
    std::string result;
    for (const auto& d : diagnostics) {
      result += (d.is_error ? "[ERROR] " : "[WARNING] ") + d.filename;
      if (d.line >= 0) result += ":" + std::to_string(d.line + 1) + ":" + std::to_string(d.column + 1);
      result += ": " + d.message + "\n";
    }
    return result;
  }
};
class FdrpcTool {
public:
  using Message = google::protobuf::Message;
  using Method  = google::protobuf::MethodDescriptor;
  using Field   = google::protobuf::FieldDescriptor;
  using Clock   = std::chrono::steady_clock;
  struct CallHandle {
    uint16_t                 rid;
    std::unique_ptr<Message> arg;
    std::unique_ptr<Message> ret;
    const Method*            desc     = nullptr;
    bool                     complete = true;
    Clock::time_point        deadline;
  };
  struct ErrHandle {
    uint16_t         id;
    uint16_t         rid;
    Rpc::DispatchSts err;
  };
  struct NotificationHandle {};
  Rpc::RpcType              last_type;
  CallHandle                call_handle;
  std::chrono::milliseconds timeout{3000};
  ErrHandle                 err_handle;
  NotificationHandle        notify_handle;
private:
  ForgeTool&                                            fg;
  ErrorCollector                                        error_collector;
  std::unique_ptr<google::protobuf::compiler::Importer> importer;
  google::protobuf::DynamicMessageFactory               factory;
  const google::protobuf::FileDescriptor*               fdesc = nullptr;
  //"Generic" render states
  std::string              err;
  std::vector<std::string> dispatch_err;
  bool                     waiting           = false;
  int                      selected_service_ = 0;
  int                      selected_method_  = 0;
public:
  FdrpcTool(ForgeTool& fg_, std::vector<std::filesystem::path> paths, const std::filesystem::path file) : fg(fg_) {
    google::protobuf::compiler::DiskSourceTree source_tree;
    for (const auto& path : paths) source_tree.MapPath("", path.string());
    importer = std::make_unique<google::protobuf::compiler::Importer>(&source_tree, &error_collector);
    fdesc    = importer->Import(file);
    if (error_collector.diagnostics.size() > 0) {
      err = error_collector.toString();
      err.push_back('\0');
    }
    call_handle.complete = true;
    call_handle.rid      = 0;
  }
  const Method*   methodSelection();
  static bool     DrawOneofInput(Message* msg, const Field* field);
  static void     DrawFieldInputRepeated(Message* msg, const Field* field);
  static void     DrawFieldInput(Message* msg, const Field* field);
  static void     DrawMessageInput(Message* msg, const std::string& name = "");
  static void     DrawMessageOutput(Message* msg, const std::string& name = "");
  static uint32_t getRpcId(const Method* method);
  static bool     deserialize(Message* msg, const std::vector<uint8_t>& data);
  void            update();
  bool            Call(std::vector<uint8_t>& data);
  Rpc::RpcType    Process(std::span<uint8_t> data);
  void            DrawError(ImVec2 dim);
  bool            renderRpcEx(ImVec2 dim);
  bool            renderRpcRet(ImVec2 dim);
};
