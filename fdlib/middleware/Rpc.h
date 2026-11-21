#pragma once
#include <stdint.h>
#include <etl/span.h>
#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>


namespace Rpc
{
using idType = uint16_t;

enum DispatchStatus : uint8_t {
    DispatchStatus_OK = 0,
    DispatchStatus_INVALID_ARGUMENTS = 1,
    DispatchStatus_NOT_FOUND = 2,
    DispatchStatus_EXECUTION_ERROR = 3,
    DispatchStatus_INTERNAL_ERROR = 4,
    DispatchStatus_REGISTRY_INVALID = 5,
    DispatchStatus_RET_STACK = 6,
};

struct DispatchStack
{
    idType id;
    idType req_id;
    etl::span<uint8_t> arg;
    etl::span<uint8_t> ret;
};

struct Method {
    const idType id;
    const pb_msgdesc_t* arg;
    const pb_msgdesc_t* ret;
    DispatchStatus (*cb)(DispatchStack stack);
};

class BinarySearch
{
    public:
    template<typename T>
    static T* find(etl::span<T> registry, idType id)
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
        size_t rx_pos = 0;
        written = 0;
        auto &id = stack.id;
        auto &req_id = stack.req_id;
        DispatchStatus sts;
        readRaw(rx, rx_pos, id);
        readRaw(rx, rx_pos, req_id);
        const Method* method = SearchPolicy::find(registry, id);
        if(!method)
            sts = DispatchStatus_NOT_FOUND;
        else
        {
            sts = decode(method->arg, rx.subspan(rx_pos), stack.arg.data());
            if(sts == DispatchStatus_OK)
                sts = method->cb(stack); 
            if(sts == DispatchStatus_OK)
            {
                size_t tx_pos = 0;
                writeRaw(tx, tx_pos, id);
                writeRaw(tx, tx_pos, req_id);
                uint8_t header_sz = tx_pos;
                size_t ret_sz = 0;
                sts = encode(method->ret, stack.ret.data(), tx.subspan(tx_pos), ret_sz);
                written = header_sz + ret_sz;
            }
        }
        if(sts != DispatchStatus_OK)
            sts = error(id, req_id, sts, tx, written);
        return sts;
    };
    DispatchStatus notify(etl::span<uint8_t> tx, DispatchStack stack, size_t& written)
    {
        const Method* method = SearchPolicy::find(registry, stack.id);
        if(!method)
            return DispatchStatus_NOT_FOUND;
        uint16_t tx_pos = 0;
        writeRaw(tx, tx_pos, stack.id);
        writeRaw(tx, tx_pos, stack.req_id);
        return encode(method->ret, stack.ret.data(), tx.subspan(tx_pos), written);
    }
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
    DispatchStatus error(idType id, idType req_id, DispatchStatus err, etl::span<uint8_t> buf, size_t &written)
    {
        constexpr size_t header_size = sizeof(idType) * 3 + sizeof(DispatchStatus);
        if (buf.size() < header_size)
            return DispatchStatus_INTERNAL_ERROR;
        size_t offset = 0;
        idType zero = 0;
        writeRaw(buf, offset, zero);
        writeRaw(buf, offset, id);
        writeRaw(buf, offset, req_id);
        writeRaw(buf, offset, err);
        written = offset;
        return err;
    }
    template <typename T>
    bool writeRaw(etl::span<uint8_t> buffer, size_t& offset, const T& value)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        if (offset + sizeof(T) > buffer.size())
            return false;
        memcpy(buffer.data() + offset, &value, sizeof(T));
        offset += sizeof(T);
        return true;
    }
    template <typename T>
    bool readRaw(etl::span<const uint8_t> buffer, size_t& offset, T& value)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        if (offset + sizeof(T) > buffer.size())
            return false;
        memcpy(&value, buffer.data() + offset, sizeof(T));
        offset += sizeof(T);
        return true;
    }
};



};
