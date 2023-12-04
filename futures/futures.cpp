#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

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

void save_to_file(const std::string& filename)
{
    std::cout << "Saving to file: " << filename << std::endl;

    std::this_thread::sleep_for(3s);

    std::cout << "File saved: " << filename << std::endl;
}

class SquareCalc
{
    std::promise<int> promise_result_;

public:
    std::future<int> get_future()
    { 
        return promise_result_.get_future();
    }

    void calculate(int x)
    { 
        try
        {
            int result = calculate_square(x);
            promise_result_.set_value(result);
        }
        catch(...)
        {
            promise_result_.set_exception(std::current_exception());
        }
    }
};

template <typename Task>
auto spawn_task(Task task)
{
    using T = decltype(task());
    std::packaged_task<T()> f_wrapped{std::forward<Task>(task)};
    auto f_result = f_wrapped.get_future();

    std::jthread thd{std::move(f_wrapped)};
    thd.detach();

    return f_result;
}

int main()
{
    std::future<int> f_square4 = std::async(std::launch::async, calculate_square, 4);
    std::future<int> f_square16 = std::async(std::launch::async, [] { return calculate_square(16); });
    std::future<int> f_square13 = std::async(std::launch::async, calculate_square, 13);
    std::future<void> f_save = std::async(std::launch::async, save_to_file, "data.dat");

    while(f_save.wait_for(100ms) != std::future_status::ready)
    {
        std::cout << ".";
        std::cout.flush();
    }

    std::cout << "square(4) = " << f_square4.get() << "\n";
    std::cout << "square(16) = " << f_square16.get() << "\n";

    try
    {
        int sq_13 = f_square13.get();
    }
    catch(const std::runtime_error& e)
    {
        std::cout << "Caught an error: " << e.what() << std::endl;
    }
    

    f_save.wait();

    {
        auto f1 = std::async(std::launch::async, save_to_file, "file1");
        auto f2 = std::async(std::launch::async, save_to_file, "file2");
        auto f3 = std::async(std::launch::async, save_to_file, "file3");
    }

    {
        auto f1 = spawn_task([] { save_to_file("file1.txt"); });
        auto f2 = spawn_task([] { save_to_file("file2.txt"); });
        auto f3 = spawn_task([] { save_to_file("file3.txt"); });

        f1.wait();
        f2.wait();
        f3.wait();
    }

    std::cout << "#######################\n";

    SquareCalc calculator;

    auto f_calc = calculator.get_future();

    std::jthread thd1{ [&calculator] { calculator.calculate(42); }};

    std::cout << "Calc result: " << f_calc.get() << "\n";

    std::packaged_task<int(int)> f_wrapped{calculate_square};
    auto f_sync = f_wrapped.get_future();

    std::jthread thd2{std::move(f_wrapped), 42};

    std::cout << "f_sync: " << f_sync.get() << "\n";
}
