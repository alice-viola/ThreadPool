#define DEBUG
#include "threadpool.hpp"
#include "test.hpp"

void
print() {
    std::cout << "Ciao " << std::endl;
}

int 
main() {
    ThreadPoolTest test = ThreadPoolTest();
    test.do_job(10000);
    test.exc_all();
    auto tp = ThreadPool(2);
    tp << [](){print();} << [](){print();};
    tp.wait();
    return 0;
}

