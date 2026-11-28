#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
class IComm {
public:
  virtual ~IComm() = default;
  // Data transfer
  virtual bool   Write(std::span<const uint8_t> data) = 0;
  virtual size_t Read(std::span<uint8_t> data)        = 0;
  virtual size_t Available() const                    = 0;
  // Events
  std::function<void()>    OnConnect;
  std::function<void()>    OnDisconnect;
  std::function<void(int)> OnError;
};