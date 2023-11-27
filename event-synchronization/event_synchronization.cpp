#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

class Data
{
    std::vector<int> data_;
    // TODO

public:
    void read()
    {
        std::cout << "Start reading..." << std::endl;
        data_.resize(100);

        std::random_device rnd;
        std::generate(begin(data_), end(data_), [&rnd] { return rnd() % 1000; });
        std::this_thread::sleep_for(2s);
        std::cout << "End reading..." << std::endl;
    }

    void process(int id)
    {
        long sum = std::accumulate(begin(data_), end(data_), 0L);
        std::cout << "Id: " << id << "; Sum: " << sum << std::endl;
    }
};

int main()
{
    {
        Data data;
        std::jthread thd_producer{[&data] { data.read(); }};

        std::jthread thd_consumer_1{[&data] { data.process(1); }};
        std::jthread thd_consumer_2{[&data] { data.process(2); }};
    }

    std::cout << "END of main..." << std::endl;
}
