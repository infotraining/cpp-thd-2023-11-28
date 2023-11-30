#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark_all.hpp>
#include "sp_sc_queue.hpp"
#include <algorithm>
#include <cassert>
#include <deque>
#include <fstream>
#include <iostream>
#include <list>
#include <random>
#include <thread>
#include <vector>

using namespace std;

constexpr int n = 100'000;

TEST_CASE("SPSC Queue")
{
    std::vector<uint64_t> data(n);
    std::random_device rd;
    std::mt19937_64 rnd_gen(rd());
    std::uniform_int_distribution<uint64_t> distr(0, 1000);
    std::generate_n(begin(data), n, [&]
        { return distr(rnd_gen); });

    BENCHMARK("with locks")
    {
        WithLocking::SingleProducerSingleConsumerQueue<uint64_t, n> queue;

        std::atomic<uint64_t> items_processed{};
        auto data_size = data.size();

        thread consumer_thd([&queue, &items_processed, data_size]
            {
            size_t local_items_processed = 0;
            while (local_items_processed < data_size)
            {
                uint64_t value;
                if (queue.try_deque(value))
                {
                    ++local_items_processed;
                }
            } 

            items_processed = local_items_processed; });

        // producer
        for (auto& item : data)
        {
            while (!queue.try_enque(item))
                continue;
        }

        consumer_thd.join();

        return items_processed.load();
    };

    BENCHMARK("lock free")
    {
        LockFree::SingleProducerSingleConsumerQueue<uint64_t, n> queue;

        std::atomic<uint64_t> items_processed{};
        auto data_size = data.size();

        thread consumer_thd([&queue, &items_processed, data_size]
            {
            size_t local_items_processed = 0;
            while (local_items_processed < data_size)
            {
                uint64_t value;
                if (queue.try_deque(value))
                {
                    ++local_items_processed;
                }
            } 

            items_processed = local_items_processed; });

        // producer
        for (auto& item : data)
        {
            while (!queue.try_enque(item))
                continue;
        }

        consumer_thd.join();

        return items_processed.load();
    };
}