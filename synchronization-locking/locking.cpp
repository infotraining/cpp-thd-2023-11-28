#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

using namespace std::literals;

template <typename Value_>
struct Synchronized
{
    Value_ value;
    std::mutex mtx_value;

    template <typename F>
    decltype(auto) with_lock(F&& function)
    {
        std::lock_guard lk{mtx_value};
        return function(value);
    }

    [[nodiscard]] std::unique_lock<std::mutex> with_lock()
    {
        return std::unique_lock(mtx_value);
    }
};

void may_throw()
{
    // throw std::runtime_error("ERROR#42");
}

void run(Synchronized<int>& value)
{
    for(int i = 0; i < 10'000'000; ++i)
    {
        //std::scoped_lock lk{mtx_value}; // begin of critical section  // mtx_value.lock()
        //++value;        
        //may_throw(); 
        //    
        value.with_lock([](int& v) { ++v; may_throw(); }); // lambda called in critical section

        auto lk = value.with_lock(); // begin of critical section
        ++value.value;

    } // end of critical section // mtx_value.unlock()
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;

    /*int counter = 0;
    std::mutex mtx_counter;*/
    Synchronized<int> counter;
    
    {
        std::jthread thd_1{[&] { run(counter); }};
        std::jthread thd_2{[&] { run(counter); }};
    }

    std::cout << "counter: " << counter.value << "\n";

    std::cout << "Main thread ends..." << std::endl;
}
