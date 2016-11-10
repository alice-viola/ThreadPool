#ifndef _THREAD_POOL_TEST_HPP_
#define _THREAD_POOL_TEST_HPP_
#ifdef __cplusplus

#include <chrono>
#include <iostream>
#include <stdlib.h>     
#include <time.h>     
#include "threadpool.hpp"

using namespace std::chrono;
using namespace astp;

/**
*   Temp solution for testing
*   ThreadPool waiting
*   UnitCpp tests.
*/
class ThreadPoolTest
{
public:
    
    ThreadPoolTest() {
        srand(time(NULL));
    };
    ~ThreadPoolTest() {};

    void 
    exc_all() {
        create(random(-100, 1000));
        resize(random(-100, 1000));
        push(random(0, 1000));
        set_sleep_time(random(-100, 1000000000));
        do_job(random(0, 1000));
    }

    void
    create(int n) {
        auto start = start_func(__func__, n);
        auto tp = ThreadPool(n);
        assert(tp.pool_size() == abs(n));
        end_func(start);
    } 

    void 
    resize(int new_size) {
        auto start = start_func(__func__, new_size);
        auto tp = ThreadPool();
        tp.resize(new_size);
        assert(tp.pool_size() == new_size || tp.pool_size() == 1);
        end_func(start);
    }

    void 
    push(int iterations) {
        auto start = start_func(__func__, iterations);
        auto tp = ThreadPool();
        tp.stop();
        for (int i = 0; i < iterations; i++) {
            tp.push([&] () { });    
        }
        assert(tp.queue_size() == iterations);
        end_func(start);
    }

    void 
    set_sleep_time(int n) {
        auto start = start_func(__func__, n);
        auto tp = ThreadPool();
        tp.set_sleep_time_ns(n);
        assert(tp.sleep_time_ns() == abs(n));
        end_func(start);
    }

    void
    do_job(int jobs_num) {
        auto start = start_func(__func__, jobs_num);
        auto tp = ThreadPool();
        for (int i = 0; i < jobs_num; i++) {
            tp.push([] () {  });    
        }
        tp.wait();
        assert(tp.queue_size() == 0);
        end_func(start);
    }

private:

    template<class ...Args>
    std::pair<std::string, high_resolution_clock::time_point>
    start_func(std::string test, Args... args) {
        std::cout << "\nStart test: " << test << " with args: ";
        print(args...);
        std::cout << std::endl;
        return std::make_pair(test, high_resolution_clock::now());
    }

    template<class T> void 
    print(T p) { 
        std::cout << p << " ";
    }

    void
    end_func(std::pair<std::string, high_resolution_clock::time_point> test) {
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        duration<double> time_span = duration_cast<duration<double>>(t2 - test.second);
        std::cout << "End test: " << test.first << " in " << time_span.count() << " seconds" << std::endl;
    }

    int
    random(int min, int max) {
        return rand() % max + min;
    }

}; // End ThreadPoolTest

#endif // __cplusplus
#endif // _THREAD_POOL_TEST_HPP_
