#pragma once
#include "Usbxch.h"
#include "TCPClient.h"
#include "Style.h"

#include <GLFW/glfw3.h>
#include <string>
#include <queue>
#include <thread>
#include <condition_variable>
#include <functional>



// Callback to handle GLFW errors
static inline void glfw_error_callback(int error, const char* description) { std::cerr << "GLFW Error " << error << ": " << description << std::endl; }

class Launchable
{
    private :
    bool exec_running = false;
    std::mutex exec_mutex;
    std::thread exec_thread;
    std::condition_variable exec_cv;
    std::queue<std::function<void()>> exec_queue;
    protected:
    virtual void onInit() {}
    virtual void onDeinit() {}
    GLFWwindow* window = nullptr;
    public:
    virtual ~Launchable()
    {
        deinit();
    }
    void init()
    {   
        onInit();
        exec_running = true;
        exec_thread = std::thread([this](){
            while (exec_running)
            {
                std::function<void()> task;
                {
                    std::unique_lock lock(exec_mutex);
                    exec_cv.wait(lock, [this]()
                    {
                        return !exec_queue.empty() || !exec_running;
                    });
                    if (!exec_running && exec_queue.empty())
                        break;
                    task = std::move(exec_queue.front());
                    exec_queue.pop();
                }
                if (task)
                {
                    try
                    {
                        task();
                    }
                    catch (const std::exception &e)
                    {
                        // Log exception
                    }
                }
            }
        });
    }
    virtual void render() = 0;
    void deinit()
    {
        {
            std::lock_guard lock(exec_mutex);
            exec_running = false;
        }
        exec_cv.notify_one();
        if (exec_thread.joinable())
        exec_thread.join();
        onDeinit();
    }
    void exec(std::function<void()> task)
    {
        {
            std::lock_guard lock(exec_mutex);
            exec_queue.push(std::move(task));
        }
        exec_cv.notify_one();
    }
    bool shouldClose() const
    {
        return window && glfwWindowShouldClose(window);
    }
};



class ForgeTool : public Launchable
{
    std::vector<std::unique_ptr<Launchable>> launchables;
    ImGuiContext* gui_ctx;
    ImPlotContext* plt_ctx;
    void onInit() override;
    void onDeinit() override;
    void render() override;
    public:
    void run();
};
