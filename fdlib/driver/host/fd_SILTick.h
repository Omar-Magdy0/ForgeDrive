#pragma once

#include <stdint.h>
#include <thread>
#include <vector>
#include <functional>
#include <atomic>
#include <chrono>
#include <queue>
#include <assert.h>

class SILTick {
    public:
    enum class Mode {
        INTERNAL, // SILTick owns the ticking thread
        EXTERNAL  // Caller owns simulation advancement
    };
    private:
    struct Event {
        uint64_t reg_ns;      
        uint64_t cb_ns; 
        uint64_t seq;
        std::function<void()> cb;
    };
    struct EventCompare
    {
        bool operator()(const Event& a, const Event& b) const
        {
            if (a.cb_ns != b.cb_ns)
                return a.cb_ns > b.cb_ns;

            return a.seq > b.seq;
        }
    };
    Mode mode_;
    uint64_t now_ns = 0;
    uint64_t next_seq = 0;
    std::thread worker_;
    std::mutex mutex_;
    std::atomic<bool> running_{false};
    inline static SILTick* instance_ = nullptr;
    std::priority_queue<Event, std::vector<Event>, EventCompare> events;

    void advance(uint64_t t)
    {
        uint64_t final_time;
        {
            std::lock_guard lock(mutex_);
            final_time = now_ns + t;
        }
        while (true)
        {
            Event e;
            {
                std::lock_guard lock(mutex_);
                if (events.empty() || events.top().cb_ns > final_time)
                {
                    now_ns = final_time;
                    break;
                }
                e = events.top();
                events.pop();
                now_ns = e.cb_ns;
            }
            if (e.cb)
                e.cb();
        }
    }
    void run()
    {
        using namespace std::chrono;
        auto next = steady_clock::now();

        while (running_)
        {
            next += milliseconds(1);
            advance(1'000'000);
            std::this_thread::sleep_until(next);
        }
    }
    SILTick(Mode mode) : mode_(mode)
    {
        if (mode_ == Mode::INTERNAL)
        {
            running_ = true;
            worker_ = std::thread(&SILTick::run, this);
        }
    }
    public:
    static void init(Mode mode)
    {
        if (!instance_)
            instance_ = new SILTick(mode);
    }
    static SILTick& instance()
    {
        assert(instance_);
        return *instance_;
    }
    void register_event(uint64_t delay_ns, std::function<void()> cb)
    {
        std::lock_guard lock(mutex_);
        events.push({
            now_ns,
            now_ns + delay_ns,
            next_seq++,
            std::move(cb)
        });
    }
    void advance_ext(uint64_t t)
    {
        assert(mode_ == Mode::EXTERNAL);
        advance(t);
    }
    uint64_t now(){return now_ns;}
    bool empty() const
    {
        return events.empty();
    }
};