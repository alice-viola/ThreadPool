#include "threadpool.hpp"

#ifndef USE_CHRONO
#define USE_CHRONO 1
#endif
#if USE_CHRONO
    #include <chrono>
    using namespace std::chrono;

    high_resolution_clock::time_point 
    start_time() {
        return high_resolution_clock::now();
    }
    
    double 
    time_since(high_resolution_clock::time_point t1) {
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
        return time_span.count();
    }
#endif

int print_num(int n) {
    std::vector<int> a;
    for (int i = 0; i < (n + 10000); i++) {
        a.push_back(i);
    }
    a.swap(a);
    return a.size();
}

class JobTest {
public:
    static int do_heavy_calc_st(int n) {
        std::vector<int> a;
        for (int i = 0; i < (n + offset); i++) {
            a.push_back(i);
        }
        a.swap(a);
        return a.size();
    }

    int do_heavy_calc(int n) {
        return JobTest::do_heavy_calc_st(n);
    }

private:
    static const int offset = 10000;
};

void 
test_thread_pool(int imax = 100000) {
    using namespace astp;
    std::cout << "\nSTART POOL" << std::endl;
    auto time = start_time(); 
    auto tp = ThreadPool<int, int>();        
    for (int i = 0; i < imax; i++) {
        auto job = JobTest();
        tp.push(std::bind(&JobTest::do_heavy_calc, job, std::placeholders::_1), i);
    }
    tp.wait();
    std::cout << "TIME POOL: " << time_since(time) << std::endl;
}

void
test_spawn_threads(int imax = 100000) {
    std::cout << "\nSTART SPAWN" << std::endl;
    auto time = start_time(); 
    std::vector< std::future< int > > threads;
    for (int i = 0; i < imax; i++) {
        auto job = JobTest();
        threads.push_back(std::async(std::launch::async, &JobTest::do_heavy_calc, job, i));
    }
    for (auto &t : threads) {
        t.get();
    }
    std::cout << "TIME SPAWN: " << time_since(time) << std::endl;
}

void 
test_sync(int imax = 100000) {
    std::cout << "\nSTART SYNC" << std::endl;
    auto time = start_time(); 
    for (int i = 0; i < imax; i++) {
        auto job = JobTest();
        job.do_heavy_calc(i);
    }
    std::cout << "TIME SYNC: " << time_since(time) << std::endl;
}

int 
main() {
    using namespace astp;
    int imax = 100000;
    for (int i = 0; i < 1; i++) {
        test_thread_pool(imax);    
    }
    test_spawn_threads(imax);
    test_sync(imax);
    return 0;
}

