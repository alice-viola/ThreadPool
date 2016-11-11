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
*/
tp.wait();
```