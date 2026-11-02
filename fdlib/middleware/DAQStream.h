#pragma once
#include "DAQCodec.h"
#include <etl/span.h>
namespace daq
{

    class Stream;
    class Channel;
    class Session;
    enum struct DAQ_ERR {

    };

    class Channel
    {
        friend Stream;
        friend Session;
        uint8_t id;
    public:
        const char *name;
        float scale;
        float offset;
    };

    class Stream
    {
        friend Session;
        int32_t ticks;
        int32_t sample_time_ns;
        uint8_t id;
        etl::span<Channel> channels;
        const char *name;
        Stream() = delete;
    public:
        Stream(etl::span<Channel> channels_, const char *name): channels(channels_), name(name){}
    };

    class Session
    {
    protected:
        uint8_t id;
        const char *name;
        etl::span<Stream> streams;
        Session() = delete;
    public:
        Session(const char *name) : name(name){}
        void registerStreams(etl::span<Stream> streams_){streams = streams_;}
        uint8_t to_write();
        uint8_t read();
        void annonate();
        DAQ_ERR discovery_req();
        DAQ_ERR discovery_respond();
        uint8_t process(const uint8_t *data, uint16_t data_len);
    };
};


