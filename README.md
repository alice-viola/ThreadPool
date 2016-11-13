#ThreadPool

A thread safe pool using C++11 features.

Features:

* Task insertion via lambda expressions
* Resize of the pool at runtime
* Support virtually an infinite number of threads in the pool
* Wait and Stop methods
* Thread safe [under testing]
* Single header [threadpool.hpp]

The ThreadPool is tested under macOS Sierra 10.12,

## Usage example 
```C++
using namespace astp;
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
using namespace astp;
/**
*	Initialize.
*/
ThreadPool tp = ThreadPool();

/**
*	Initialize with 64
*	threads.
*/
ThreadPool tp = ThreadPool(64);  

/**
*	Resize to 
*	32 threads.
*	Wait the end of execution
*	if the removed threads are running
*	some jobs.
*	Callable from every thread.
*/
tp.resize(32);
tp.resize(128);

/**
*	Push.
*	Callable from every thread.
*/
tp.push([&]() {
	// Push something
});

/**
*	Wait the end of execution
*	of all the jobs in the queue. 
*	Callable from every thread.
*/
tp.wait();

/**
*	Stop.
*	Wait the end of execution
*	if the removed threads are running
*	some jobs.
*	Callable from every thread.
*/
tp.stop();

/**
*	Threads sleep time
*	in nanoseconds.
*	A big value make the 
*	threadpool less energy expensive
*	at the expense of minors performances.
*	Callable from every thread.
*/
tp.set_sleep_time_ns(100000000);
auto sleep_time = tp.sleep_time_ns();

/**
*	Synchronize.
*	Under the hood is implemented
*	as binary semaphore.
*	Callable from every thread.
*/
tp.synchronize();
tp.end_synchronize();

/**
*	Misc
*	Callable from every thread.
*/
auto current_pool_size = tp.pool_size();
auto current_queue_size = tp.queue_size(); 
auto is_empty = tp.queue_is_empty();
```

## Example with synchronize
```C++
ThreadPool tp = ThreadPool(); 
auto data = std::vector<int>();
for (int i = 0; i < 100000; i++) {
    tp.push([i]() {
        auto double_num = i * 2;
        /**
        *	The call to synchronize
        *	allow the safe insertion
        *	in the data container [common for all the threads] 
        *	of the thread result. 
        */
        tp.synchronize();
        /**
        *	Insert values in data.
        */ 
        data.push_back(double_num);
        /**
        *	Signal the end of the insertion,
        *	so others threads can inserts
        *	values in the container.
        */
        tp.end_synchronize();
    });
}
tp.wait();
for (auto &d : data) 
    std::cout << d << std::endl;
```

## To Do
I'm working on the exception handling.