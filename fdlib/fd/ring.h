#pragma once
#include <stdint.h>
#include <assert.h>
#include <etl/span.h>

#ifdef __cplusplus
class UnitTest;
namespace fd
{
    template <typename T>
    class ring_fast
    {
        friend UnitTest;
        size_t capacity_;
        size_t mask_;

    public:
        ring_fast(T *buffer, size_t size_) : capacity_(size_), head_(0), tail_(0), buffer_(buffer)
        {
            assert(capacity_ > 0);
            assert((capacity_ & (capacity_ - 1)) == 0);
            assert(capacity_ <= 32768);
            mask_ = capacity_ - 1;
        }
        ring_fast(etl::span<T> buffer) : capacity_(buffer.size()), head_(0), tail_(0), buffer_(buffer.data())
        {
            assert(capacity_ > 0);
            assert((capacity_ & (capacity_ - 1)) == 0);
            assert(capacity_ <= 32768);
            mask_ = capacity_ - 1;
        }

        bool isPowerOfTwo(size_t x)
        {
            return x && ((x & (x - 1)) == 0);
        }

        // Push element - ISR safe (single producer)
        // Returns false if full
        bool push(const T &item)
        {
            uint16_t next = (head_ + 1) & mask_;
            if (next == tail_)
            {
                return false;
            }
            buffer_[head_] = item;
            head_ = next;
            return true;
        }
        void pushFast(const T &item)
        {
            buffer_[head_] = item;
            head_ = (head_ + 1) & mask_;
        }

        // Pop element - ISR safe (single consumer)
        // Returns false if empty
        bool pop(T &item)
        {
            if (head_ == tail_)
            {
                return false;
            }
            item = buffer_[tail_];
            tail_ = (tail_ + 1) & mask_;
            return true;
        }
        bool peek(T &item)
        {
            if (head_ == tail_)
            {
                return false;
            }
            item = buffer_[tail_];
            return true;
        }
        bool peek(T &item, uint16_t idx) const
        {
            if (idx >= capacity_)
                return false;
            item = buffer_[idx];
            return true;
        }
        bool advance(uint16_t n)
        {
            if (n > size())
                return false;
            tail_ = (tail_ + n) & mask_;
            return true;
        }
        bool distanceFromTail(size_t idx, size_t &distance)
        {
            if (idx >= capacity_)
                return false;

            // Distance from tail to idx (modulo buffer size)
            uint32_t d = (idx - tail_) & mask_;

            // Is idx inside the valid FIFO contents?
            if (d >= size())
                return false;

            distance = d;
            return true;
        }

        void clear()
        {
            head_ = 0;
            tail_ = 0;
        }

        bool empty() const { return head_ == tail_; }
        bool full() const { return ((head_ + 1) & mask_) == tail_; }
        size_t free() const { return (capacity() - size()); }
        size_t size() const { return (head_ - tail_) & mask_; }
        size_t capacity() const { return capacity_ - 1; }

        // Multibyte Reserve/Commit pattern
        //  Reserve space for writing 'count' elements.
        // Returns false if insufficient space.
        bool write_reserve(
            size_t count,
            T **writeptr1,
            size_t *cont1,
            T **writeptr2,
            size_t *cont2)
        {
            if (count > (capacity() - this->size()))
            {
                *writeptr1 = nullptr;
                *writeptr2 = nullptr;
                *cont1 = 0;
                *cont2 = 0;
                return false;
            }

            size_t head = head_;
            size_t tail = tail_;
            size_t cont_space;

            if (head >= tail)
                cont_space = capacity_ - head - (tail == 0 ? 1 : 0);
            else
                cont_space = tail - head - 1;

            *writeptr1 = &buffer_[head];

            if (cont_space >= count)
            {
                *writeptr2 = nullptr;
                *cont1 = count;
                *cont2 = 0;
            }
            else
            {
                *writeptr2 = &buffer_[0];
                *cont1 = cont_space;
                *cont2 = count - cont_space;
            }

            return true;
        }
        void write_commit(size_t count)
        {
            head_ = (head_ + count) & mask_;
        }
        bool read_reserve(
            T **readptr1,
            size_t *cont1,
            T **readptr2,
            size_t *cont2)
        {
            size_t cnt = this->size();

            if (cnt == 0)
            {
                *readptr1 = nullptr;
                *readptr2 = nullptr;
                *cont1 = 0;
                *cont2 = 0;
                return false;
            }

            size_t cont_space;

            if (tail_ >= head_)
                cont_space = capacity_ - tail_;
            else
                cont_space = head_ - tail_;

            *readptr1 = &buffer_[tail_];

            if (cont_space >= cnt)
            {
                *readptr2 = nullptr;
                *cont1 = cnt;
                *cont2 = 0;
            }
            else
            {
                *readptr2 = &buffer_[0];
                *cont1 = cont_space;
                *cont2 = cnt - cont_space;
            }

            return true;
        }
        void read_commit(size_t count)
        {
            tail_ = (tail_ + count) & mask_;
        }
        size_t tail() const { return tail_; }
        size_t head() const { return head_; }

    private:
        volatile size_t head_;
        volatile size_t tail_;
        T *buffer_;
    };

    template <typename T>
    class ring
    {
        friend UnitTest;

        size_t capacity_;

        volatile size_t head_;
        volatile size_t tail_;

        T *buffer_;

        size_t increment(size_t index, size_t amount = 1) const
        {
            index += amount;

            if (index >= capacity_)
                index -= capacity_;

            return index;
        }

    public:
        ring(T *buffer, size_t size)
            : capacity_(size),
              head_(0),
              tail_(0),
              buffer_(buffer)
        {
            assert(capacity_ > 1);
        }

        ring(etl::span<T> buffer)
            : capacity_(buffer.size()),
              head_(0),
              tail_(0),
              buffer_(buffer.data())
        {
            assert(capacity_ > 1);
        }

        // Push element - ISR safe (single producer)
        // Returns false if full
        bool push(const T &item)
        {
            size_t next = increment(head_);

            if (next == tail_)
                return false;

            buffer_[head_] = item;
            head_ = next;

            return true;
        }

        // Push without checking for available space.
        // Caller must guarantee the buffer is not full.
        void pushFast(const T &item)
        {
            buffer_[head_] = item;
            head_ = increment(head_);
        }

        // Pop element - ISR safe (single consumer)
        // Returns false if empty
        bool pop(T &item)
        {
            if (head_ == tail_)
                return false;

            item = buffer_[tail_];
            tail_ = increment(tail_);

            return true;
        }

        // Peek next element without removing it.
        bool peek(T &item)
        {
            if (head_ == tail_)
                return false;

            item = buffer_[tail_];

            return true;
        }

        // Peek element at an absolute buffer index.
        bool peek(T &item, size_t idx) const
        {
            if (idx >= capacity_)
                return false;

            item = buffer_[idx];

            return true;
        }

        // Advance read position by n elements.
        bool advance(size_t n)
        {
            if (n > size())
                return false;

            tail_ = increment(tail_, n);

            return true;
        }

        // Find distance from tail to an absolute buffer index.
        //
        // Returns false if idx is not currently part of the
        // valid FIFO contents.
        bool distanceFromTail(int16_t idx, size_t &distance)
        {
            if (idx < 0 || idx >= capacity_)
                return false;

            size_t i = static_cast<size_t>(idx);

            size_t d;

            if (i >= tail_)
                d = i - tail_;
            else
                d = capacity_ - tail_ + i;

            if (d >= size())
                return false;

            distance = d;

            return true;
        }

        void clear()
        {
            head_ = 0;
            tail_ = 0;
        }

        bool empty() const
        {
            return head_ == tail_;
        }

        bool full() const
        {
            return increment(head_) == tail_;
        }

        size_t free() const
        {
            return capacity() - size();
        }

        size_t size() const
        {
            if (head_ >= tail_)
                return head_ - tail_;

            return capacity_ - tail_ + head_;
        }

        // Number of elements that can actually be stored.
        //
        // One slot is intentionally left unused to distinguish
        // full from empty.
        size_t capacity() const
        {
            return capacity_ - 1;
        }

        // Multibyte Reserve/Commit pattern
        bool write_reserve(
            size_t count,
            T **writeptr1,
            size_t *cont1,
            T **writeptr2,
            size_t *cont2)
        {
            if (count > (capacity() - size()))
            {
                *writeptr1 = nullptr;
                *writeptr2 = nullptr;
                *cont1 = 0;
                *cont2 = 0;

                return false;
            }

            size_t head = head_;
            size_t tail = tail_;

            size_t cont_space;

            if (head >= tail)
            {
                cont_space = capacity_ - head;

                // Keep one slot empty if tail is at zero.
                if (tail == 0)
                    --cont_space;
            }
            else
            {
                cont_space = tail - head - 1;
            }

            *writeptr1 = &buffer_[head];

            if (cont_space >= count)
            {
                *writeptr2 = nullptr;
                *cont1 = count;
                *cont2 = 0;
            }
            else
            {
                *writeptr2 = &buffer_[0];
                *cont1 = cont_space;
                *cont2 = count - cont_space;
            }

            return true;
        }

        void write_commit(size_t count)
        {
            head_ = increment(head_, count);
        }

        bool read_reserve(
            T **readptr1,
            size_t *cont1,
            T **readptr2,
            size_t *cont2)
        {
            size_t cnt = size();

            if (cnt == 0)
            {
                *readptr1 = nullptr;
                *readptr2 = nullptr;
                *cont1 = 0;
                *cont2 = 0;

                return false;
            }

            size_t cont_space;

            if (tail_ >= head_)
                cont_space = capacity_ - tail_;
            else
                cont_space = head_ - tail_;

            *readptr1 = &buffer_[tail_];

            if (cont_space >= cnt)
            {
                *readptr2 = nullptr;
                *cont1 = cnt;
                *cont2 = 0;
            }
            else
            {
                *readptr2 = &buffer_[0];
                *cont1 = cont_space;
                *cont2 = cnt - cont_space;
            }

            return true;
        }

        void read_commit(size_t count)
        {
            tail_ = increment(tail_, count);
        }

        size_t tail() const
        {
            return tail_;
        }

        size_t head() const
        {
            return head_;
        }
    };
}

#else

#endif