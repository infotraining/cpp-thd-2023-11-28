#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <stop_token>
#include <vector>
#include <syncstream>

using namespace std::literals;

auto sync_cout()
{
    return std::osyncstream(std::cout);
}

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay)
{
    std::osyncstream(std::cout) << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        sync_cout() << "bw#" << id << ": " << c << std::endl;

        std::this_thread::sleep_for(delay);
    }

    sync_cout() << "bw#" << id << " is finished..." << std::endl;
}

void background_work_stoppable(std::stop_token stp_token, size_t id, const std::string& text, std::chrono::milliseconds delay)
{
    sync_cout() << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        sync_cout() << "bw#" << id << ": " << c << std::endl;

        if (stp_token.stop_requested())
        {
            sync_cout() << "Stop has been requested for THD#" << id << "..." << std::endl;
            sync_cout() << "THD#" << id << " has been cancelled..." << std::endl;
            return;
        }

        std::this_thread::sleep_for(delay);
    }

    sync_cout() << "bw#" << id << " is finished..." << std::endl;
}

class BackgroundWork
{
    const int id_;
    const std::string text_;

public:
    BackgroundWork(int id, std::string text)
        : id_{id}
        , text_{std::move(text)}
    {
    }

    void operator()(std::chrono::milliseconds delay) const
    {
        sync_cout() << "BW#" << id_ << " has started..." << std::endl;

        for (const auto& c : text_)
        {
            sync_cout() << "BW#" << id_ << ": " << c << std::endl;

            std::this_thread::sleep_for(delay);
        }

        sync_cout() << "BW#" << id_ << " is finished..." << std::endl;
    }
};

int main()
{
    sync_cout() << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    std::thread thd_empty;
    sync_cout() << "thd_empty.get_id() = " << thd_empty.get_id() << "\n";

    std::thread thd_1(&background_work, 1, "THREAD#1", 250ms);

    BackgroundWork bw{2, "BackgroundWorker"};
    std::thread thd2{std::ref(bw), 350ms};
    thd2.detach();
    assert(thd2.get_id() == std::thread{}.get_id());

    std::thread thd_3{
        [text]() {
            background_work(3, text, 100ms);
        }};

    std::thread thd_4 = std::move(thd_3);

    thd_3 = std::thread{[]() { background_work(4, "THD#4", 100ms); }};

    std::jthread thd_5{[] {
        background_work(5, "JTHREAD#5", 2s);
    }};

    std::stop_source stp_source;
    std::stop_token stp_token = stp_source.get_token();

    std::jthread thd_6([=]() { background_work_stoppable(stp_token, 42, "STOPPABLE", 500ms); });
    std::jthread thd_7([=]() { background_work_stoppable(stp_token, 665, "stoppable", 500ms); });
    std::jthread thd_8(&background_work_stoppable, 667, "CANCELABLE", 500ms);

    if (thd_3.joinable())
        thd_3.join();

    thd_1.join();
    thd_4.join();

    stp_source.request_stop(); // stops threads 6 & 7
    thd_8.request_stop();
    
    sync_cout() << "Main thread ends..." << std::endl;
}
