#ThreadPool

A thread safe pool using C++14 features.

Features:

* Task insertion via lambda expressions
* Resize of the pool at runtime
* Support virtually an infinite number of threads in the pool
* Wait and Stop methods
* Thread safe [under testing]
* Single header [threadpool.hpp]

## Usage example 
```C++
/**
*	Create a pool with the default 
*	architecture dependent number
* 	of threads.
*/
ThreadPool tp = ThreadPool(); 
for (int i = 0; i < 100; i++) {
	/**
	*	Push load in the pool queue 
	*	via lambda.
	*/
    tp.push([]() {
        std::vector<int> vec;
        for (int k = 0; k < 1000; k++) {
            vec.push_back(k);
            std::cout << vec.size() << std::endl;
        }
    });
}
/**
*	Wait until all jobs are done.
*	Not mandatory, is up to you.
*/
tp.wait();
```

## Public methods examples
```C++
/**
*	Initialize
*/
ThreadPool tp = ThreadPool();
ThreadPool tp = ThreadPool(64);  

/**
*	Resize
*/
tp.resize(64);

/**
*	Push
*/
tp.push([&]() {
	// Push something
});

/**
*	Wait
*/
tp.wait();

/**
*	Stop
*/
tp.stop();

/**
*	Threads sleep time
*	in nanoseconds.
*/
tp.set_sleep_time_ns(100'000'000);
auto sleep_time = tp.sleep_time_ns();

/**
*	Synchronize
*/
tp.synchronize();
tp.end_synchronize();

/**
*	Misc
*/
auto current_pool_size = tp.pool_size();
auto current_queue_size = tp.queue_size(); 
auto is_empty = tp.queue_is_empty();
```

## Example with sync
```C++
ThreadPool tp = ThreadPool(); 
auto data = std::vector<int>();
for (int i = 0; i < 100'000; i++) {
    tp.push([i]() {
        auto double_num = i * 2;
        tp.synchronize();
        data.push_back(double_num);
        tp.end_synchronize();
    });
}
tp.wait();
for (auto &d : data) 
    std::cout << d << std::endl;
}
```