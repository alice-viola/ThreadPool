#include "test.hpp"

int 
main() {
    auto tpp = ThreadPoolTest();
    tpp.exc_all();
    auto tp = ThreadPool(64);
    auto vec = std::vector<int>(600);
    int i = 0;
    tp.apply_for(600, [&vec, &i]() {
        vec[i] = i;
        i++;
    });
    for (auto &v : vec) {
        std::cout << v << std::endl;
    }
    return 0;
}

