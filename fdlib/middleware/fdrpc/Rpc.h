#pragma once
#include <stdint.h>
#include <etl/span.h>
#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include "RpcDef.h"

namespace Rpc
{
struct DispatchStack
{
    uint16_t id;
    uint16_t rid;
    etl::span<uint8_t> arg;
    etl::span<uint8_t> ret;
};

struct Method {
    const uint16_t id;
    const pb_msgdesc_t* arg;
    const pb_msgdesc_t* ret;
    DispatchStatus (*cb)(DispatchStack stack);
};

class BinarySearch
{
    public:
    template<typename T>
    static T* find(etl::span<T> registry, uint16_t id)
    {
        size_t L = 0;
        size_t R = registry.size();
        while(L != R)
        {
            auto m = L + (R-L)/2;
            if (registry[m].id < id)
                L = m + 1;
            else
                R = m;
        }
        return (L < registry.size() && registry[L].id == id) ? &registry[L] : nullptr;
    }
};

template<typename SearchPolicy = BinarySearch>
class RpcServer
{
private:
    etl::span<const Method>  registry;
public:
    RpcServer(etl::span<const Method> _registry): registry(_registry){}
    DispatchStatus validate()
    {
        //Validate registry (no null methods, ordered ascending, etc)
        for(size_t i = 0; i < registry.size(); i++)
        {
            const bool out_of_order = (i > 0) && (registry[i].id <= registry[i - 1].id);
            if(!registry[i].cb || !registry[i].ret || !registry[i].arg || out_of_order)
                return DispatchStatus_REGISTRY_INVALID;
        }
        return DispatchStatus_OK;
    }
    DispatchStatus dispatch(etl::span<uint8_t> rx, etl::span<uint8_t> tx, DispatchStack stack, size_t& written)
    {
        written = 0;
        uint16_t &id = stack.id;
        uint16_t &rid = stack.rid;
        DispatchStatus sts;
        if(rx.size() < 4)
            return DispatchStatus_EXECUTION_ERROR;
        id = static_cast<uint16_t>(rx[0]) |  static_cast<uint16_t>(rx[1] << 8); 
        rid = static_cast<uint16_t>(rx[2]) |  static_cast<uint16_t>(rx[3] << 8); 
        const Method* method = SearchPolicy::find(registry, id);
        if(!method)
            sts = DispatchStatus_NOT_FOUND;
        else
        {
            sts = decode(method->arg, rx.subspan(4), stack.arg.data());
            if(sts == DispatchStatus_OK)
                sts = method->cb(stack); 
            if(sts == DispatchStatus_OK)
            {
                tx[0] = static_cast<uint8_t>(id);
                tx[1] = static_cast<uint8_t>(id >> 8);
                tx[2] = static_cast<uint8_t>(rid);
                tx[3] = static_cast<uint8_t>(rid >> 8);
                size_t ret_sz = 0;
                sts = encode(method->ret, stack.ret.data(), tx.subspan(4), ret_sz);
                written = 4 + ret_sz;
            }
        }
        if(sts != DispatchStatus_OK)
            sts = error(id, rid, sts, tx, written);
        return sts;
    };
    DispatchStatus encode(const pb_msgdesc_t* desc, const void* src, etl::span<uint8_t> output, size_t& written)
    {
        pb_ostream_t stream = pb_ostream_from_buffer(output.data(), output.size());
        bool status = pb_encode(&stream, desc, src);
        written = stream.bytes_written;
        return status?DispatchStatus_OK:DispatchStatus_INTERNAL_ERROR;
    }
    DispatchStatus decode(const pb_msgdesc_t* desc, etl::span<const uint8_t> input, void* dst)
    {
        pb_istream_t stream = pb_istream_from_buffer(input.data(), input.size());
        bool status = pb_decode(&stream, desc, dst);
        return status ? DispatchStatus_OK : DispatchStatus_INVALID_ARGUMENTS;
    }
    DispatchStatus error(uint16_t id, uint16_t rid, DispatchStatus err, etl::span<uint8_t> buf, size_t &written)
    {
        constexpr size_t header_size = sizeof(uint16_t) * 3 + sizeof(uint8_t);
        if (buf.size() < header_size)
            return DispatchStatus_INTERNAL_ERROR;
        buf[0] = 0;
        buf[1] = 0;
        buf[2] = static_cast<uint8_t>(id);
        buf[3] = static_cast<uint8_t>(id >> 8);
        buf[4] = static_cast<uint8_t>(rid);
        buf[5] = static_cast<uint8_t>(rid >> 8);
        buf[6] = static_cast<uint8_t>(err);
        written = header_size;
        return err;
    }
};



};
