#ifndef SINGLE_PRODUCER_SINGLE_CONSUMER_QUEUE
#define SINGLE_PRODUCER_SINGLE_CONSUMER_QUEUE

#include <array>
#include <atomic>
#include <mutex>

namespace WithLocking
{
    template <typename T, unsigned int N>
    class SingleProducerSingleConsumerQueue
    {
        std::array<T, N> buffer_;
        unsigned int head_{0};
        unsigned int tail_{0};
        std::mutex mtx_;

    public:
        SingleProducerSingleConsumerQueue() = default;

        SingleProducerSingleConsumerQueue(const SingleProducerSingleConsumerQueue&) = delete;
        SingleProducerSingleConsumerQueue& operator=(const SingleProducerSingleConsumerQueue&) = delete;

        bool try_enque(const T& item) // producer
        {
            std::lock_guard<std::mutex> lk{mtx_};

            if (tail_ == head_ + N) // buffer is full
                return false;

            buffer_[tail_++ % N] = item; // enque an item in the buffer
            return true;
        }

        bool try_deque(T& item) // consumer
        {
            std::lock_guard<std::mutex> lk{mtx_};

            if (tail_ == head_) // buffer is empty
                return false;

            item = buffer_[head_++ % N]; // deque from the buffer

            return true;
        }
    };
}

namespace LockFree
{
    template <typename T, unsigned int N>
    class SingleProducerSingleConsumerQueue
    {
        std::array<T, N> buffer_;
        std::atomic<unsigned int> head_{0};
        std::atomic<unsigned int> tail_{0};

    public:
        SingleProducerSingleConsumerQueue() = default;

        SingleProducerSingleConsumerQueue(const SingleProducerSingleConsumerQueue&) = delete;
        SingleProducerSingleConsumerQueue& operator=(const SingleProducerSingleConsumerQueue&) = delete;

        bool try_enque(const T& item) // producer
        {
            auto tail = tail_.load(std::memory_order_relaxed); // load a position of tail

            if (head_.load(std::memory_order_acquire) + N == tail) // buffer is full
                return false;

            buffer_[tail % N] = item; // write to buffer
            tail_.store(tail + 1, std::memory_order_release); // update tail

            return true;
        }

        bool try_deque(T& item) // consumer
        {
            auto head = head_.load(std::memory_order_relaxed);

            if (tail_.load(std::memory_order_acquire) == head) // queue is empty
                return false;

            item = buffer_[head % N];
            head_.store(head + 1, std::memory_order_release); // update head

            return true;
        }
    };
}

#endif //SINGLE_PRODUCER_SINGLE_CONSUMER_BUFFER
