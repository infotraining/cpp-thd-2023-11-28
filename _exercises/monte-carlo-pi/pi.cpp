#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <execution>
#include <future>
#include <iostream>
#include <latch>
#include <numeric>
#include <random>
#include <thread>

/*******************************************************
 * https://academo.org/demos/estimating-pi-monte-carlo
 * *****************************************************/

using namespace std;

const uintmax_t N = 100'000'000;

void calc_hits(const uintmax_t count, uintmax_t& hits)
{
    std::mt19937_64 rnd_gen(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    std::uniform_real_distribution<double> rnd(0, 1.0);

    uintmax_t local_hits{};
    for (uintmax_t n = 0; n < count; ++n)
    {
        double x = rnd(rnd_gen);
        double y = rnd(rnd_gen);
        if (x * x + y * y < 1)
            hits++;
    }
}

void calc_hits_local_counter(const uintmax_t count, uintmax_t& hits)
{
    std::mt19937_64 rnd_gen(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    std::uniform_real_distribution<double> rnd(0, 1.0);

    uintmax_t local_hits{};
    for (uintmax_t n = 0; n < count; ++n)
    {
        double x = rnd(rnd_gen);
        double y = rnd(rnd_gen);
        if (x * x + y * y < 1)
            local_hits++;
    }

    hits += local_hits;
}

void single_thread_pi()
{
    //////////////////////////////////////////////////////////////////////////////
    // single thread

    cout << "Pi calculation started! Single thread!" << endl;
    const auto start = chrono::high_resolution_clock::now();

    uintmax_t hits = 0;

    calc_hits(N, hits);

    const double pi = static_cast<double>(hits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed = " << elapsed_time << "ms" << endl;
}

void pi_many_threads_hot_loop()
{
    cout << "Pi calculation started! Many threads - hot loop" << endl;
    const auto start = chrono::high_resolution_clock::now();

    uintmax_t hits = 0;
    const unsigned int number_of_cores = std::thread::hardware_concurrency();
    const uintmax_t chunk_size = N / number_of_cores;

    std::vector<uintmax_t> hits_vec(number_of_cores, 0);
    std::vector<std::thread> threads;

    for (int i = 0; i < number_of_cores; ++i)
    {
        threads.push_back(std::thread{&calc_hits, chunk_size, std::ref(hits_vec[i])});
    }

    for (auto& thread : threads)
        if (thread.joinable())
            thread.join();

    hits = std::accumulate(hits_vec.begin(), hits_vec.end(), 0L);

    const double pi = static_cast<double>(hits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed = " << elapsed_time << "ms" << endl;
}

void pi_many_threads_local_counter()
{
    cout << "Pi calculation started! Many threads - local counter" << endl;
    const auto start = chrono::high_resolution_clock::now();

    uintmax_t hits = 0;
    const unsigned int number_of_cores = std::thread::hardware_concurrency();
    const uintmax_t chunk_size = N / number_of_cores;

    std::vector<uintmax_t> hits_vec(number_of_cores, 0);
    std::vector<std::thread> threads;

    for (int i = 0; i < number_of_cores; ++i)
    {
        threads.push_back(std::thread{&calc_hits_local_counter, chunk_size, std::ref(hits_vec[i])});
    }

    for (auto& thread : threads)
        if (thread.joinable())
            thread.join();

    hits = std::accumulate(hits_vec.begin(), hits_vec.end(), 0L);

    const double pi = static_cast<double>(hits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed = " << elapsed_time << "ms" << endl;
}

struct Hits
{
    alignas(std::hardware_destructive_interference_size) uintmax_t value;
};

void calc_hits_with_padding(const uintmax_t count, Hits& hits)
{
    std::mt19937_64 rnd_gen(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    std::uniform_real_distribution<double> rnd(0, 1.0);

    for (uintmax_t n = 0; n < count; ++n)
    {
        double x = rnd(rnd_gen);
        double y = rnd(rnd_gen);
        if (x * x + y * y < 1)
            hits.value++;
    }
}

void calc_hits_with_atomic(const uintmax_t count, std::atomic<uintmax_t>& hits)
{
    std::mt19937_64 rnd_gen(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    std::uniform_real_distribution<double> rnd(0, 1.0);

    uintmax_t local_counter{};
    for (uintmax_t n = 0; n < count; ++n)
    {
        double x = rnd(rnd_gen);
        double y = rnd(rnd_gen);
        if (x * x + y * y < 1)
            local_counter++;
    }

    //hits += local_counter;
    hits.fetch_add(local_counter, std::memory_order_relaxed);
}

void pi_with_atomic()
{
    cout << "Pi calculation started! Atomics!" << endl;
    const auto start = chrono::high_resolution_clock::now();

    std::atomic<uintmax_t> hits = 0;
    const unsigned int number_of_cores = std::thread::hardware_concurrency();
    const uintmax_t chunk_size = N / number_of_cores;

    std::vector<std::thread> threads;

    for (int i = 0; i < number_of_cores; ++i)
    {
        threads.push_back(std::thread{&calc_hits_with_atomic, chunk_size, std::ref(hits)});
    }

    for (auto& thread : threads)
        if (thread.joinable())
            thread.join();    

    const double pi = static_cast<double>(hits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed = " << elapsed_time << "ms" << endl;
}

void pi_with_padding()
{
    cout << "Pi calculation started! Padding!" << endl;
    const auto start = chrono::high_resolution_clock::now();

    uintmax_t hits = 0;
    const unsigned int number_of_cores = std::thread::hardware_concurrency();
    const uintmax_t chunk_size = N / number_of_cores;

    std::vector<Hits> hits_vec(number_of_cores);
    std::vector<std::thread> threads;

    for (int i = 0; i < number_of_cores; ++i)
    {
        threads.push_back(std::thread{&calc_hits_with_padding, chunk_size, std::ref(hits_vec[i])});
    }

    for (auto& thread : threads)
        if (thread.joinable())
            thread.join();

    hits = std::accumulate(hits_vec.begin(), hits_vec.end(), 0L, [](uintmax_t total, Hits hits) { return total + hits.value; });

    const double pi = static_cast<double>(hits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed = " << elapsed_time << "ms" << endl;
}

int main()
{
    single_thread_pi();

    std::cout << "-------------\n";

    pi_many_threads_hot_loop();

    std::cout << "-------------\n";

    pi_many_threads_local_counter();

    std::cout << "-------------\n";

    pi_with_padding();
    
    std::cout << "-------------\n";

    pi_with_atomic();
}