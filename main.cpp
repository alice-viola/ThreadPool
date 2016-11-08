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

int 
main() {
    using namespace astp;
    auto time = start_time(); 
    auto tp = ThreadPool<int, int>();
    std::cout <<"Init pool size: " << tp.pool_size() << std::endl;
    std::cout <<"Init queue size: " << tp.queue_size() << std::endl;
    for (int i = 0; i < 1000; i++) {
        auto job = JobTest();
        tp.push(std::bind(&JobTest::do_heavy_calc, job, std::placeholders::_1), i);
    }
    tp.stop(true);
    std::cout << "TIME : " << time_since(time) << std::endl;
    return 0;
}



