#include "threadpool.hpp"
#include <iostream>

using namespace astp;

int main() {
    ThreadPool tp; 
    for (int i = 0; i < 2; i++) {
        tp.push([i]() {
            std::cout << "ThreadPool " << i << std::endl; 
        });
    }
    tp.wait();
    return 0;
}