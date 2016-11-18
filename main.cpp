#include "test.hpp"

int 
main() {
    auto t = ThreadPoolTest();
    t.exc_all();
    
    auto tp = ThreadPool();
    std::function<void(std::string)> efunc = [](std::string e) { 
        std::cout << "Caught exception " << e << std::endl;
    };
    tp.set_excpetion_action(efunc);
    for (int i = 0; i < 10; i++) {
        tp << [i](){ throw std::to_string(i); };
    }
    tp.wait();
    
    auto val = tp.future_from_push([]() -> int {
        int counter = 1;
        for (int i = 1; i < 100000; i++) {
            counter++;
        }
        return counter;
    });
    auto val2 = tp.future_from_push([]() -> std::string {
        return "ciao";
    });
    val.wait();
    val2.wait();
    auto val1 = val.get();
    auto val22 = val2.get();
    std::cout <<  val1 << std::endl;
    std::cout << val22 << std::endl;

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

