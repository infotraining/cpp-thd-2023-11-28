#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

using namespace std::literals;

void run(int& value)
{
    for(int i = 0; i < 10'000'000; ++i)
    {
        ++value;
    }
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;

    int counter = 0;
    
    {
        std::jthread thd_1{[&counter] { run(counter); }};
        std::jthread thd_2{[&counter] { run(counter); }};
    }

    std::cout << "counter: " << counter << "\n";

    std::cout << "Main thread ends..." << std::endl;
}
