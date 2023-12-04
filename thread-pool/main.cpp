#include "thread_safe_queue.hpp"

#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using Task = std::function<void()>;

namespace PoisoningPill
{

    const Task EndOfWork{};

    class ThreadPool
    {
        ThreadSafeQueue<Task> tasks_;
        std::vector<std::jthread> threads_;

        void run()
        {
            while (true)
            {
                Task task;
                tasks_.pop(task);
                if (!task)
                    break;
                task();
            }
        }

    public:
        ThreadPool(size_t size = std::thread::hardware_concurrency())
            : threads_(size)
        {
            for (auto& thd : threads_)
                thd = std::jthread{[this] {
                    run();
                }};
        }

        ~ThreadPool()
        {
            for (size_t i = 0; i < threads_.size(); ++i)
                tasks_.push(EndOfWork);

            for (auto& thd : threads_)
                thd.join();
        }

        void submit(Task task)
        {
            if (!task)
                throw std::invalid_argument{"Empty task not allowed"};

            std::cout << "ThreadPool::submit" << std::endl;
            tasks_.push(std::move(task));
        }
    };

} // namespace PoisoningPill

class ThreadPool
{
    ThreadSafeQueue<Task> tasks_;
    std::vector<std::jthread> threads_;
    std::atomic<bool> end_of_work_{};

    void run()
    {
        while (true)
        {
            if (end_of_work_)
                break;
            
            Task task;
            tasks_.pop(task);            
            task();
        }
    }

public:
    ThreadPool(size_t size = std::thread::hardware_concurrency())
        : threads_(size)
    {
        for (auto& thd : threads_)
            thd = std::jthread{[this] {
                run();
            }};
    }

    ~ThreadPool()
    {
        for (size_t i = 0; i < threads_.size(); ++i)
            tasks_.push([this] { end_of_work_ = true; });

        for (auto& thd : threads_)
            thd.join();
    }

    void submit(Task task)
    {
        std::cout << "ThreadPool::submit" << std::endl;
        tasks_.push(std::move(task));
    }
};

using namespace std::literals;

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay)
{
    std::cout << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        std::cout << "bw#" << id << ": " << c << std::endl;

        std::this_thread::sleep_for(delay);
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    {
        ThreadPool thd_pool(std::thread::hardware_concurrency());

        thd_pool.submit([] { background_work(1, "Hello", 250ms); });

        for (int i = 2; i < 30; ++i)
        {
            thd_pool.submit([i] { background_work(i, "BW#"s + std::to_string(i), 350ms); });
        }
    }

    std::cout << "Main thread ends..." << std::endl;
}
