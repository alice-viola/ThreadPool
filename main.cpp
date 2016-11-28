#include "threadpool.hpp"
#include <iostream>

using namespace astp;

int 
main() {
    ThreadPool tp(8);
    
    tp << []() { std::cout << "Hello world!" << std::endl; };

    tp.wait();
    return 0;
}

