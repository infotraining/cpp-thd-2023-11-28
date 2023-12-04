#include "thread_safe_queue.hpp"

#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

int calculate_square(int x)
{
    std::cout << "Starting calculation for " << x << " in " << std::this_thread::get_id() << std::endl;

    std::random_device rd;
    std::uniform_int_distribution<> distr(100, 5000);

    std::this_thread::sleep_for(std::chrono::milliseconds(distr(rd)));

    if (x % 13 == 0)
        throw std::runtime_error("Error#13");

    return x * x;
}

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

    template <typename Function>
    auto submit(Function&& f)
    {
        using T = decltype(f());

        auto f_wrapped = std::make_shared<std::packaged_task<T()>>(std::forward<Function>(f));
        auto f_result = f_wrapped->get_future();
        tasks_.push([f_wrapped] { (*f_wrapped)(); });

        return f_result;
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

    std::vector<std::future<int>> f_squares;

    {
        ThreadPool thd_pool(std::thread::hardware_concurrency());

        thd_pool.submit([] { background_work(1, "Hello", 250ms); });

        

        for (int i = 2; i < 30; ++i)
        {
            auto f = thd_pool.submit([i] { return calculate_square(i); });
            f_squares.push_back(std::move(f));
        }

        std::cout << "\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" << std::endl;

        for (auto& fs : f_squares)
        {
            try
            {
                int s = fs.get();
                std::cout << "Result: " << s << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cout << e.what() << "\n";
            }
        }
    }

    std::cout << "Main thread ends..." << std::endl;
}
