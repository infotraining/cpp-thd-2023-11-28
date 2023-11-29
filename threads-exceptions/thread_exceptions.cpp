#include <bitset>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <variant>
#include <vector>

using namespace std::literals;

template <typename... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

template <typename T>
struct ThreadResult
{
private:
    std::variant<std::monostate, T, std::exception_ptr> value;

public:
    void set_exception(std::exception_ptr e)
    {
        value = e;
    }

    template <typename U>
    void set_value(U&& u)
    {
        value = std::forward<U>(u);
    }

    T get()
    {
        std::visit(overloaded{
                       [](const T& v) -> T {
                           return v;
                       },
                       [](std::exception_ptr e) -> T {
                           std::rethrow_exception(e);
                       },
                       [](std::monostate) -> T {
                           throw std::logic_error("ThreadResult not set");
                       }}, value);
    }
};

void background_work(size_t id, const std::string& text, ThreadResult<char>& result)
{
    std::cout << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        std::cout << "bw#" << id << ": " << c << std::endl;

        std::this_thread::sleep_for(100ms);
    }

    try
    {
        result.set_value(text.at(5)); // potential exception
        std::bitset<8> bs{"0101012"};
    }
    catch (...)
    {
        result.set_exception(std::current_exception());
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    // char c;
    // std::exception_ptr eptr;

    ThreadResult<char> result;

    std::jthread thd_1{
        [&result]() {
            background_work(1, "HelloWorld", result);
        }};

    thd_1.join(); ////////////////////////////////////////////

    try
    {
        char c = result.get();
        std::cout << "Letter: " << c << std::endl;
    }
    catch (const std::out_of_range& e)
    {
        std::cout << "Caught: " << e.what() << std::endl;
    }
    catch (std::exception& e)
    {
        std::cout << "Bitset: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "Last hope!!!" << std::endl;
    }

    std::cout << "Main thread ends..." << std::endl;
}
