#include "threadpool.hpp"
#include "test.hpp"

int 
main() {
    auto tp = ThreadPool();
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

