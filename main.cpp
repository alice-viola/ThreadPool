#include "threadpool.hpp"
#include "test.hpp"
#include <fstream>

int
write(int i) {
    std::ofstream myfile;
    myfile.open ("./testoutput/example" + std::to_string(i) + (".txt"));
    for(int k = 0; k < 1000000; k++) 
        myfile << "Writing this to a file.\n";
    myfile.close();
    return i;
}

void 
testfunc() {
    std::cout << __func__ <<std::endl; 
}

using namespace astp;

void 
add(int i) {
    int a = i + 1; 
}


void 
example() {
    ThreadPool tp;

    tp.push([](){
        std::cout << "ciao" << std::endl;
    });

    tp.push([](){}, [](){}, [](){}, [](){}, [](){});

    tp << [](){} << [](){} << testfunc;


    auto fut = tp.future_from_push([](){ return 66; });
    fut.wait();
    std::cout << "Res: " << fut.get() << std::endl;


    tp.dg_open("writetest");
    std::cout << "A" << std::endl;
    for (int i = 0; i < 100; i++) {
        tp.dg_insert("writetest", [i](){ add(i); });
    }
    try {
        tp.dg_open("writetest");
    } catch(std::runtime_error e) {
        std::cout << e.what() << std::endl;
    }
    tp.dg_open("writetest2");
    tp.dg_close_with_barrier("writetest", [&](){
        std::cout << "Write finished, restart" << std::endl;
        for (int i = 0; i < 100; i++) {
            tp.dg_insert("writetest2", [i](){ add(i); });
        }
        tp.dg_close_with_barrier("writetest2", [](){
            std::cout << "Write2 finished" << std::endl;
        });
    });
    
    tp.dg_now("test", []() {
        std::cout << "I'm executed now!" << std::endl;
    });

    tp.apply_for(10, [](){
        std::cout << "Hey!" << std::endl;
    });


    tp.stop();
    tp.awake();

    std::function<void(std::string)> efunc = [](std::string e) { 
        std::cout << "Caught exception " << e << std::endl;
    };
    tp.set_excpetion_action(efunc);
    tp << [](){ throw std::to_string(56); };   
    tp.dg_wait("writetest2");
}

int 
main() {
    test_threadpool();
    example();
    return 0;
}

