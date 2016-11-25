#define DEBUG
#include "threadpool.hpp"
#include "test.hpp"
#include <iostream>

#include <chrono>
#include <fstream>

using namespace std::chrono;

template<class ...Args>
std::pair<std::string, high_resolution_clock::time_point>
start_func(std::string test, Args... args) {
    std::cout << "\nStart test: " << test << " with args: ";
    std::cout << std::endl;
    return std::make_pair(test, high_resolution_clock::now());
}
void
end_func(std::pair<std::string, high_resolution_clock::time_point> test) {
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - test.second);
    std::cout << "End test: " << test.first << " in " << time_span.count() << " seconds" << std::endl;
}

int
write(int i) {
    std::ofstream myfile;
    myfile.open ("/Users/adda/Work/ThreadPool/testoutput/example" + std::to_string(i) + (".txt"));
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
    tp.set_sleep_time_ns(0);


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
    auto test = ThreadPoolTest();
    test.exc_all();
    //example();


    std::vector<int> vec = std::vector<int>();
    for (int i = 0; i < 100; i++) {
        vec.push_back(i);
    }
    
    int k = 0;
    /*auto time2 = start_func("normal", "noargs");
    auto newvec2 = std::vector<int>(vec.size());
    for (auto &v : vec) {
        newvec2[k] = write(k);
        k++;
    }
    end_func(time2);*/

    ThreadPool tp(8);
    tp.set_sleep_time_ns(0);

    auto time3 = start_func("apply", "noargs");
    int i = 0;
    auto newvec3 = std::vector<int>(vec.size());
    tp.apply_for(100, [&vec, &newvec3, i](){
        newvec3[i] = write(vec[i]);
    });
    end_func(time3);

    auto time31 = start_func("apply async", "noargs");
    i = 0;
    auto newvec31 = std::vector<int>(vec.size());
    tp.apply_for_async(100, [&vec, &newvec31, i](){
        newvec31[i] = write(vec[i]);
    });
    tp.wait();
    end_func(time31);


    auto time5 = start_func("normal_push", "noargs");
    auto newvec4 = std::vector<int>(vec.size());
    k = 0;
    for (int i = 0; i < 100; i++) {
        tp << [&k, &vec, &newvec4]() { 
            newvec4[k] = write(vec[k]);
            k++;
        };
    }
    tp.wait();
    end_func(time5);

    auto time6 = start_func("futures", "noargs");
    k = 0;
    auto futs = std::vector<std::future<int> >(vec.size());
    auto newvec5 = std::vector<int>(vec.size());
    for (int i = 0; i < vec.size(); i++) {
        futs[i] = tp.future_from_push([&k]() -> int {
            int a = write(k);
            k++;
            return a;
        });
    }
    for (int i = 0; i < vec.size(); i++) {
        futs[i].wait();
        newvec5[i] = futs[i].get();
    }
    end_func(time6);


    std::function<void(std::string)> efunc = [](std::string e) { 
        std::cout << "Caught exception " << e << std::endl;
    };
    tp.set_excpetion_action(efunc);
    for (int i = 0; i < 10; i++) {
        tp << [i](){ throw std::to_string(i); };
    }
    tp.wait();    
    return 0;
}

