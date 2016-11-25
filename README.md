#ThreadPool

A thread safe pool using C++11 features.
Developed with three main functionalities in mind: 

1. Provide fast methods in order to run tasks with high priority:
    * *apply_for* methods
    * *push* methods

2. Provide methods for handle complex multithread apps, like Apple's GCD:
    * *dispatch_group* methods

3. Be scalable, safe and simple:
    * Resize at run-time
    * Stop and awake
    * User inputs sanity checks (can be disabled)
    * A lot of synchronizations methods ready to use
    * Thread safe
    * Excpetion handling
    * Single header

## Features:

* Standard C++11
* Task insertion via lambda expressions
* Task insertion via functions
* Futures from push
* Resize of the pool at runtime
* Support virtually an infinite number of threads in the pool
* Stop and awake the pool
* Fluent-Interface for task insertion
* Fast methods for high priority tasks
* Dispatch groups methods
* Barriers methods
* Synchronizations methods
* Thread safe 
* Single header [threadpool.hpp]
* MIT license

The ThreadPool is tested under macOS Sierra 10.12 (Apple LLVM version 7.2.0 (clang-702.0.25)), 
LinuxMint 17.1 Rebecca (g++ 4.8.2), Apple iOS 9.3 (XCode 8.0, LLVM version 8.0). 

## Compile
You can run some example called in the provided main.cpp file
with the following bash lines, or simply include the *threadpool.hpp* file
in your projects.

for macOS (and OSX as well):
```bash
g++ -std=c++11 -O3 main.cpp
```

for Linux:
```bash
g++ -std=c++11 -O3 -pthread main.cpp
```

The -O3 optimization parameter is obviously optional.

## First contact
```C++
#include "threadpool.hpp"
#include <iostream>

using namespace astp;

ThreadPool tp = ThreadPool(); 
for (int i = 0; i < 100; i++) {
    tp.push([i]() {
        std::cout << "ThreadPool " << i << std::endl; 
    });
}
tp.wait();
```
## API

### Initialization
You can create the thread pool with the default platform dependent number of threads, or
you can specify your desired number: at least one thread must be created.
```C++
auto tp = astp::ThreadPool();   // -> Create default pool
auto tp = astp::ThreadPool(64); // -> Create 64 threads
auto tp = astp::ThreadPool(0);  // -> Throw an error
auto tp = astp::ThreadPool(-1); // -> Throw an error
```
### Resize
The pool can be resized after it was created: if the resizing operation decreases
the current number of threads, a number equal to the difference is popped from 
the pool, but only when the threads have finished to compute their workload.
At least one thread is must be kept in the pool.
During the resizing, the stop of the pool is blocked.
```C++
// For instace, current pool size is 64.
tp.resize(31) // -> Pop  (64 - 31) = 33 threads
// For instace, current pool size is 64.
tp.resize(74) // -> Push (74 - 64) = 10 threads
tp.resize(0)  // -> Throw an error
tp.resize(-1) // -> Throw an error
```

### Insertion of tasks
There are three basic syntax for task insertion in the pool, all of them
uses lambda expression. The task inserted are appended at the end of a queue.
```C++
// Classic syntax
tp.push([ /* Lambda capturing */ ] () { /* Task */ });

// Variadic template syntax
tp.push(
[ /* Lambda capturing */ ] () { /* Task1 */ },
[ /* Lambda capturing */ ] () { /* Task2 */ },
[ /* Lambda capturing */ ] () { /* Task3 */ }
);

// Overload operator << syntax
tp << []() { /* Some jobs */};
```
All these syntax have a fluent-interface, so you can chain the calls:

```C++
tp.push([](){}).push([](){}).push([](){});
tp << []() { /* Some jobs1 */} << []() { /* Some jobs2 */};
```
When you push a task, the task is inserted in queue, and it will be
executed when it will be at the front of the queue. 
See the next section for how to wait the execution of the task.

You can also pass directly a function, but this function must *return void*
and have *no arguments*.

```C++
void func() { /* Do stuff */ }
tp.push(func);
```

### Waiting execution
When the tasks are inserted in the pool, you cannot know when
they will be completed. If you need to know when they are completed,
you can wait the execution of all the tasks with the following method.
This will wait, blocking the caller thread, until all the tasks have finished
to run.
```C++
tp.wait();
```

### Stop the pool
With this method all the threads in the pool will be 
stopped, waiting the end of task execution, and then will
be popped. So at end of the stop exceution, the thread pool
will have zero threads. During the stop, the resize of the pool
is blocked.
```C++
tp.stop();
```

### Awake the pool
After that the *stop* method is called, the pool has zero threads,
so new tasks will not be executed.
So you need to call the *awake* method: the pool will be resized to the same number
of threads that the pool had before stopping.
```C++
tp.awake();
```

### Apply [a.k.a how to run stuff FASTER]
This method allow you to run a repetitive task a lot faster than
the above methods: 
*apply_for* let you specify a task and a number of times that this
task must be executed, and return only when the entire task is finished.
The tasks inserted with this method have the max priority, and are
inserted in the front of the queue [That is actually a std::deque]. 
Furthermore, the mutex that controls the queue access is acquired only once,
than all tasks are inserted in the queue; this saves the lock/unlock time.
```C++
auto vec = std::vector<int>(600);
int i = 0;
tp.apply_for(600, [&vec, &i]() {
    vec[i] = doStuff();
    i++;
});
// Return only when all the iterations
// will be executed.
```
I have experimented a performance boost from 10% to 300% using this method instead
of the classical push.

There is also the async version, that acts like the normal push.
```C++
auto vec = std::vector<int>(600);
int i = 0;
tp.apply_for_async(600, [&vec, &i]() {
    vec[i] = doStuff();
    i++;
});
// Returns immediately

These functions throws an error if the iteration counts is less than zero.
```

### Future from push
For task insertion, you may like to get a future reference to the pushed 
job. This feature was inspired by vit-vit threadpool.  

```C++
auto future_value = tp.future_from_push([]() -> std::string {
    return "Hello world!";
});
future_value.wait();
auto value = future_value.get();
std::cout << value << std::endl // --> Hello world!
```

### Dispatch Groups
You may have the need of track a series of jobs, so
the thread pool has some methods to accomplish that.
Dispatch group introduce a little overhead, so you should
use it only when you really need to track some tasks.
Start using them by creating a group with a string id,
then append some tasks, and signal the end of the insertion.
When you signal the end of insertion, tasks will be moved
to the pool queue. Than you can wait until they are computed.
```C++
// Create a group named "group_id"
tp.dg_open("group_id");
// Insert tasks in the group.
tp.dg_insert("group_id", []() { /* task1 */ });
tp.dg_insert("group_id", []() { /* task2 */ });
// Signal the end of task insertion.
tp.dg_close("group_id");  
// Signal the end of task insertion and add a barrier.
tp.dg_close_with_barrier("group_id", [](){});  
// Wait the end of execution [if needed]
tp.dg_wait("group_id");
// Wait the end of execution [if needed], 
// and fire a callback when all tasks in the group were ran.
// Can throw.
tp.dg_wait("group_id", []() { /* Fired when the group has been entirely computed */ });
// Synchronize access to external container
tp.dg_synchronize("group_id");
tp.dg_end_synchronize("group_id");
```
Dispatch group allow the execution of a task with high priority:
the method *dg_now* will insert the task directly
at the front of the pool queue.
```C++
tp.dg_now("group_id", []() { std::cout << "High priority" << std::endl; });
```
This is useful when you have a lot of tasks in the pool queue and you want
to process something without waiting the end of all others tasks. 

All these methods throws if try to do illegal operations, like close a group that
don't exist.

### Synchronization
Thread pool has four methods that allow the synchronization of the threads in the pool
when accessing some external critical part. These methods acts with binary semaphore
implemented in a nested class of the thread pool.

Standard synchronization:
```C++
std::vector<int> data;
for (int i = 0; i < 100; i++) {
    tp.push([i, &tp, &data](){
        // Signal to all others threads in the pool
        // to wait.
        tp.synchronize(); 
        // Safely modify external container.
        data.push_back(i);
        // Signal all others thread to go on.
        tp.end_synchronize();
    }); 
}
```

Dispatch group synchronization:
```C++
std::vector<int> data;
tp.dg_open("data_group");
for (int i = 0; i < 100; i++) {
    tp.dg_insert("data_group", [i, &tp, &data](){
        // Signal to all others threads that 
        // are working for the group to wait.
        tp.dg_synchronize("data_group"); 
        // Safely modify external container.
        data.push_back(i);
        // Signal all others threads in the group to go on.
        tp.dg_synchronize("data_group");
    }); 
}
tp.dg_close("data_group");  
```

### Sleep
Every thread in the pool is running in a while loop (until you stop the pool).
This is a consuming process, so you can set the sleep time for threads
when there aren't jobs to do, so the threads in the pool will go to sleep.
An higher value of sleep makes the pool less responsive when new jobs are
inserted, so in case of performance critical tasks, you should set this
interval small. 
Throw if the interval is negative.
*Seems that the minimal interval is or zero, or a time-slice of the scheduler.*
```C++
// Set sleep in nanoseconds
tp.set_sleep_time_ns(100);
// Set sleep in milliseconds
tp.set_sleep_time_ms(100);
// Set sleep in seconds
tp.set_sleep_time_s(100);
tp.set_sleep_time_s(99.85);
// Return the current sleep time in nanoseconds
auto stns = tp.sleep_time_ns(); 
```

### Misc
Various methods in order to get information
about the state of the threadpool.
```C++
auto current_pool_size = tp.pool_size();
auto current_queue_size = tp.queue_size(); 
auto is_empty = tp.queue_is_empty();
```
### Excpetion handling 
You can set a callback the will be callled every time
one of pool threads fire an excpetion:

```C++
std::function<void(std::string)> efunc = [](std::string e) { 
    std::cout << "Caught exception " << e << std::endl;
};
tp.set_excpetion_action(efunc);
i = 26;
tp << [i](){ throw std::to_string(i); };
// -> Caught exception 26
```

If you don't set a callback the threadpool will fire the 
default one, that does nothing.
You can override this behaviour **[at your risk]** declaring the follow
macro: `#define TP_ENABLE_DEFAULT_EXCEPTION_CALL 0`

### Internal excpetions
Every method of the threadpool can throw for excpetional causes like
failed memory allocation. Futhermore, by default the input checking is active,
than the threadpool can throw some specific expections: the methods
where this behavior is expected, are marked with *noexcept(false)*.
You can disable the input checking with the following macro:
`#define TP_ENABLE_SANITY_CHECKS 0`

## Performance
This test was a write to text test: write one million of lines
in a *iterations* number of different text files.
NT means the sequential version, TP[num] means the number of
threads in the threadpool. Test was executed with the normal
*push* function.

![Test performance](images/test2.png)


Test function:
```C++
void
write(int i) {
    std::ofstream myfile;
    myfile.open ("example" + std::to_string(i) + (".txt"));
    for(int k = 0; k < 1000000; k++) 
        myfile << "Writing this to a file.\n";
    myfile.close();
}
```

## License
ThreadPool is released under the MIT license.
See the file *LICENSE.txt* for the license text.