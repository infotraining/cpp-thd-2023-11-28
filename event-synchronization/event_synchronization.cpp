#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <syncstream>
#include <thread>
#include <vector>
#include <execution>

using namespace std::literals;

auto sync_cout()
{
    return std::osyncstream(std::cout);
}

namespace Atomics
{
    namespace BusyWait
    {
        class Data
        {
            std::vector<int> data_;
            std::atomic<bool> is_data_ready_{};
            static_assert(std::atomic<bool>::is_always_lock_free);

        public:
            void read()
            {
                sync_cout() << "Start reading..." << std::endl;
                data_.resize(100);

                std::random_device rnd;
                std::generate(begin(data_), end(data_), [&rnd] { return rnd() % 1000; });
                std::this_thread::sleep_for(5s);
                sync_cout() << "End reading..." << std::endl;
                is_data_ready_.store(true, std::memory_order_release); // is_data_ready_ = true;
            }

            void process(int id)
            {
                while (!is_data_ready_.load(std::memory_order_acquire)) { } // while (!is_data_ready_) {}  // busy_wait
                long sum = std::accumulate(begin(data_), end(data_), 0L);
                sync_cout() << "Id: " << id << "; Sum: " << sum << std::endl;
            }
        };
    } // namespace BusyWait

    namespace IdleWait
    { 
        class Data
        {
            std::vector<int> data_;
            std::atomic<bool> is_data_ready_{};           

        public:
            void read()
            {
                sync_cout() << "Start reading..." << std::endl;
                data_.resize(100);

                std::random_device rnd;
                std::generate(begin(data_), end(data_), [&rnd] { return rnd() % 1000; });
                std::this_thread::sleep_for(5s);
                sync_cout() << "End reading..." << std::endl;                
                is_data_ready_ = true;
                is_data_ready_.notify_all();
            }

            void process(int id)
            {
                is_data_ready_.wait(false);
                long sum = std::accumulate(begin(data_), end(data_), 0L);
                sync_cout() << "Id: " << id << "; Sum: " << sum << std::endl;
            }
        };
    }
} // namespace Atomics

namespace CV
{
    class Data
    {
        std::vector<int> data_;
        bool is_data_ready_;
        std::mutex mtx_is_data_ready_;
        std::condition_variable cv_data_ready_;

    public:
        void read()
        {
            sync_cout() << "Start reading..." << std::endl;
            data_.resize(100'000'000);

            std::random_device rnd;
            std::generate(begin(data_), end(data_), [&rnd] { return rnd() % 1000; });
            std::this_thread::sleep_for(5s);
            sync_cout() << "End reading..." << std::endl;

            {
                std::lock_guard lk{mtx_is_data_ready_};
                is_data_ready_ = true;
            }

            cv_data_ready_.notify_all(); // send notification to all sleeping threads
        }

        void process(int id)
        {

            std::unique_lock lk{mtx_is_data_ready_};
            //while(!is_data_ready_)
            //{
            //     cv_data_ready_.wait(lk); // idle wait
            //}
            cv_data_ready_.wait(lk, [this] { return is_data_ready_; });

            lk.unlock();

            sync_cout() << "Start processing..." << std::endl;

            auto t_start = std::chrono::high_resolution_clock::now();
            std::sort(std::execution::par_unseq, begin(data_), end(data_));
            auto sum = std::reduce(std::execution::par_unseq, begin(data_), end(data_), 0ULL);            
            auto t_end = std::chrono::high_resolution_clock::now();
            sync_cout() << "Id: " << id << "; Sum: " << sum << std::endl;
            sync_cout() << "Time#" << id << " = "
                        << std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count() << "ms\n";
        }
    };
} // namespace CV

int main()
{
    {
        CV::Data data;
        std::jthread thd_producer{[&data] {
            data.read();
        }};

        std::jthread thd_consumer_1{[&data] {
            data.process(1);
        }};

        std::jthread thd_consumer_2{[&data] {
            data.process(2);
        }};
    }

    std::cout << "END of main..." << std::endl;
}
