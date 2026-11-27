#pragma once
#include <assert.h>
#include <etl/span.h>
#include <stdint.h>
#ifdef __cplusplus
class UnitTest;
namespace Fd {
template <typename T> class RingFast {
  friend UnitTest;
  size_t capacity_;
  size_t mask_;
public:
  RingFast(T* buffer, size_t size_) : capacity_(size_), head_(0), tail_(0), buffer_(buffer) {
    assert((capacity_ & (capacity_ - 1)) == 0);
    mask_ = capacity_ - 1;
  }
  RingFast(etl::span<T> buffer) : capacity_(buffer.Size()), head_(0), tail_(0), buffer_(buffer.data()) {
    assert((capacity_ & (capacity_ - 1)) == 0);
    mask_ = capacity_ - 1;
  }
  bool IsPowerOfTwo(size_t x) { return x && ((x & (x - 1)) == 0); }
  // Push element - ISR safe (single producer)
  // Returns false if full
  bool Push(const T& item) {
    uint16_t next = (head_ + 1) & mask_;
    if (next == tail_) { return false; }
    buffer_[head_] = item;
    head_          = next;
    return true;
  }
  void PushFast(const T& item) {
    buffer_[head_] = item;
    head_          = (head_ + 1) & mask_;
  }
  // Pop element - ISR safe (single consumer)
  // Returns false if empty
  bool Pop(T& item) {
    if (head_ == tail_) { return false; }
    item  = buffer_[tail_];
    tail_ = (tail_ + 1) & mask_;
    return true;
  }
  bool Peek(T& item) {
    if (head_ == tail_) { return false; }
    item = buffer_[tail_];
    return true;
  }
  bool Peek(T& item, uint16_t idx) const {
    if (idx >= capacity_) return false;
    item = buffer_[idx];
    return true;
  }
  bool Advance(uint16_t n) {
    if (n > Size()) return false;
    tail_ = (tail_ + n) & mask_;
    return true;
  }
  bool DistanceFromTail(size_t idx, size_t& distance) {
    if (idx >= capacity_) return false;
    // Distance from tail to idx (modulo buffer size)
    uint32_t d = (idx - tail_) & mask_;
    // Is idx inside the valid FIFO contents?
    if (d >= Size()) return false;
    distance = d;
    return true;
  }
  void Clear() {
    head_ = 0;
    tail_ = 0;
  }
  bool   Empty() const { return head_ == tail_; }
  bool   Full() const { return ((head_ + 1) & mask_) == tail_; }
  size_t Free() const { return (Capacity() - Size()); }
  size_t Size() const { return (head_ - tail_) & mask_; }
  size_t Capacity() const { return capacity_ - 1; }
  // Multibyte Reserve/Commit pattern
  //  Reserve space for writing 'count' elements.
  // Returns false if insufficient space.
  bool WriteReserve(size_t count, T** writeptr1, size_t* cont1, T** writeptr2, size_t* cont2) {
    if (count > (Capacity() - this->Size())) {
      *writeptr1 = nullptr;
      *writeptr2 = nullptr;
      *cont1     = 0;
      *cont2     = 0;
      return false;
    }
    size_t head = head_;
    size_t tail = tail_;
    size_t cont_space;
    if (head >= tail) cont_space = capacity_ - head - (tail == 0 ? 1 : 0);
    else cont_space = tail - head - 1;
    *writeptr1 = &buffer_[head];
    if (cont_space >= count) {
      *writeptr2 = nullptr;
      *cont1     = count;
      *cont2     = 0;
    } else {
      *writeptr2 = &buffer_[0];
      *cont1     = cont_space;
      *cont2     = count - cont_space;
    }
    return true;
  }
  void WriteCommit(size_t count) { head_ = (head_ + count) & mask_; }
  bool ReadReserve(T** readptr1, size_t* cont1, T** readptr2, size_t* cont2) {
    size_t cnt = this->Size();
    if (cnt == 0) {
      *readptr1 = nullptr;
      *readptr2 = nullptr;
      *cont1    = 0;
      *cont2    = 0;
      return false;
    }
    size_t cont_space;
    if (tail_ >= head_) cont_space = capacity_ - tail_;
    else cont_space = head_ - tail_;
    *readptr1 = &buffer_[tail_];
    if (cont_space >= cnt) {
      *readptr2 = nullptr;
      *cont1    = cnt;
      *cont2    = 0;
    } else {
      *readptr2 = &buffer_[0];
      *cont1    = cont_space;
      *cont2    = cnt - cont_space;
    }
    return true;
  }
  void   ReadCommit(size_t count) { tail_ = (tail_ + count) & mask_; }
  size_t Tail() const { return tail_; }
  size_t Head() const { return head_; }
private:
  volatile size_t head_;
  volatile size_t tail_;
  T*              buffer_;
};
template <typename T> class Ring {
  friend UnitTest;
  size_t          capacity_;
  volatile size_t head_;
  volatile size_t tail_;
  T*              buffer_;
  size_t          Increment(size_t index, size_t amount = 1) const {
    index += amount;
    if (index >= capacity_) index -= capacity_;
    return index;
  }
public:
  Ring(T* buffer, size_t size) : capacity_(size), head_(0), tail_(0), buffer_(buffer) { assert(capacity_ > 1); }
  Ring(etl::span<T> buffer) : capacity_(buffer.Size()), head_(0), tail_(0), buffer_(buffer.data()) { assert(capacity_ > 1); }
  // Push element - ISR safe (single producer)
  // Returns false if full
  bool Push(const T& item) {
    size_t next = Increment(head_);
    if (next == tail_) return false;
    buffer_[head_] = item;
    head_          = next;
    return true;
  }
  // Push without checking for available space.
  // Caller must guarantee the buffer is not full.
  void PushFast(const T& item) {
    buffer_[head_] = item;
    head_          = Increment(head_);
  }
  // Pop element - ISR safe (single consumer)
  // Returns false if empty
  bool Pop(T& item) {
    if (head_ == tail_) return false;
    item  = buffer_[tail_];
    tail_ = Increment(tail_);
    return true;
  }
  // Peek next element without removing it.
  bool Peek(T& item) {
    if (head_ == tail_) return false;
    item = buffer_[tail_];
    return true;
  }
  // Peek element at an absolute buffer index.
  bool Peek(T& item, size_t idx) const {
    if (idx >= capacity_) return false;
    item = buffer_[idx];
    return true;
  }
  // Advance read position by n elements.
  bool Advance(size_t n) {
    if (n > Size()) return false;
    tail_ = Increment(tail_, n);
    return true;
  }
  // Find distance from tail to an absolute buffer index.
  //
  // Returns false if idx is not currently part of the
  // valid FIFO contents.
  bool DistanceFromTail(int16_t idx, size_t& distance) {
    if (idx < 0 || idx >= capacity_) return false;
    size_t i = static_cast<size_t>(idx);
    size_t d;
    if (i >= tail_) d = i - tail_;
    else d = capacity_ - tail_ + i;
    if (d >= Size()) return false;
    distance = d;
    return true;
  }
  void Clear() {
    head_ = 0;
    tail_ = 0;
  }
  bool   Empty() const { return head_ == tail_; }
  bool   Full() const { return Increment(head_) == tail_; }
  size_t Free() const { return Capacity() - Size(); }
  size_t Size() const {
    if (head_ >= tail_) return head_ - tail_;
    return capacity_ - tail_ + head_;
  }
  // Number of elements that can actually be stored.
  //
  // One slot is intentionally left unused to distinguish
  // full from empty.
  size_t Capacity() const { return capacity_ - 1; }
  // Multibyte Reserve/Commit pattern
  bool WriteReserve(size_t count, T** writeptr1, size_t* cont1, T** writeptr2, size_t* cont2) {
    if (count > (Capacity() - Size())) {
      *writeptr1 = nullptr;
      *writeptr2 = nullptr;
      *cont1     = 0;
      *cont2     = 0;
      return false;
    }
    size_t head = head_;
    size_t tail = tail_;
    size_t cont_space;
    if (head >= tail) {
      cont_space = capacity_ - head;
      // Keep one slot empty if tail is at zero.
      if (tail == 0) --cont_space;
    } else {
      cont_space = tail - head - 1;
    }
    *writeptr1 = &buffer_[head];
    if (cont_space >= count) {
      *writeptr2 = nullptr;
      *cont1     = count;
      *cont2     = 0;
    } else {
      *writeptr2 = &buffer_[0];
      *cont1     = cont_space;
      *cont2     = count - cont_space;
    }
    return true;
  }
  void WriteCommit(size_t count) { head_ = Increment(head_, count); }
  bool ReadReserve(T** readptr1, size_t* cont1, T** readptr2, size_t* cont2) {
    size_t cnt = Size();
    if (cnt == 0) {
      *readptr1 = nullptr;
      *readptr2 = nullptr;
      *cont1    = 0;
      *cont2    = 0;
      return false;
    }
    size_t cont_space;
    if (tail_ >= head_) cont_space = capacity_ - tail_;
    else cont_space = head_ - tail_;
    *readptr1 = &buffer_[tail_];
    if (cont_space >= cnt) {
      *readptr2 = nullptr;
      *cont1    = cnt;
      *cont2    = 0;
    } else {
      *readptr2 = &buffer_[0];
      *cont1    = cont_space;
      *cont2    = cnt - cont_space;
    }
    return true;
  }
  void   ReadCommit(size_t count) { tail_ = Increment(tail_, count); }
  size_t Tail() const { return tail_; }
  size_t Head() const { return head_; }
};
} // namespace Fd
#else
#endif