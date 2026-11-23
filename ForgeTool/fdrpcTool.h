#pragma once
#include "ForgeTool.h"
#include <filesystem>
#include <memory>
#include <vector>
#include "fdrpc.pb.h"
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>

class ErrorCollector : public google::protobuf::compiler::MultiFileErrorCollector
{
    struct Diagnostic
    {
        bool is_error;
        std::string filename;
        int line;
        int column;
        std::string message;
    };

public:
    std::vector<Diagnostic> diagnostics;
    void AddError(const std::string &filename, int line, int column, const std::string &message) override
    {
        diagnostics.push_back({true, filename, line, column, message});
    }
    void AddWarning(const std::string &filename, int line, int column, const std::string &message) override
    {
        diagnostics.push_back({false, filename, line, column, message});
    }
    std::string toString() const
    {
        std::string result;
        for (const auto &d : diagnostics)
        {
            result += (d.is_error ? "[ERROR] " : "[WARNING] ") + d.filename;
            if (d.line >= 0)
                result += ":" + std::to_string(d.line + 1) + ":" + std::to_string(d.column + 1);
            result += ": " + d.message + "\n";
        }
        return result;
    }
};

class FdrpcTool
{
    using Message = google::protobuf::Message;
    using Method = google::protobuf::MethodDescriptor;
    using Field = google::protobuf::FieldDescriptor;

private:
    ForgeTool &fg;
    ErrorCollector error_collector;
    std::unique_ptr<google::protobuf::compiler::Importer> importer;
    google::protobuf::DynamicMessageFactory factory;
    std::unique_ptr<Message> arg;
    std::unique_ptr<Message> ret;
    std::string err;
    const google::protobuf::FileDescriptor *desc = nullptr;
    int selected_service_ = 0;
    int selected_method_ = 0;
    const Method *method_ = nullptr;

public:
    FdrpcTool(ForgeTool &fg_, std::vector<std::filesystem::path> paths, const std::filesystem::path file) : fg(fg_)
    {
        google::protobuf::compiler::DiskSourceTree source_tree;
        for (const auto &path : paths)
            source_tree.MapPath("", path.string());
        importer = std::make_unique<google::protobuf::compiler::Importer>(&source_tree, &error_collector);
        desc = importer->Import(file);
        if (error_collector.diagnostics.size() > 0)
        {
            err = error_collector.toString();
            err.push_back('\0');
        }
    }
    const Method *methodSelection();
    static bool DrawOneofInput(Message *msg, const Field *field);
    static void DrawFieldInputRepeated(Message *msg, const Field *field);
    static void DrawFieldInput(Message *msg, const Field *field);
    static void DrawMessageInput(Message *msg, const std::string &name = "");
    void render(ImVec2 dim);
};
