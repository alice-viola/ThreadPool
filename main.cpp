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
    for (int i = 0; i < 100; i++) {
        tp.push([i]() {
            std::vector<int> vec;
            for (int k = 0; k < 1000; k++) {
                vec.push_back(k);
            }
            std::cout << "Printing: " << vec.size() << std::endl;
        });
    }
    tp.dispatch_group_enter("g1");
    tp.dispatch_group_insert("g1", []() { std::cout << "GROUP1" << std::endl; });
    tp.dispatch_group_leave("g1");
    tp.dispatch_group_now("now65", []() {
        std::cout << "I'm processed now!" << std::endl;
        std::vector<int> vec;
        for (int k = 0; k < 100000; k++) {
            vec.push_back(k);
        }
        std::cout << "I'm processed now end!" << std::endl;
    });
    tp.wait();    
    return 0;
}

