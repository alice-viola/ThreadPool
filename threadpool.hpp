#ifndef _THREAD_POOL_HPP_
#define _THREAD_POOL_HPP_
#ifdef __cplusplus

#include <thread>
#include <future>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <queue>
#include <chrono>
#include <assert.h>
#include <iostream>

namespace astp {

    template<class T, class K>
    class ThreadPool
    {
    public:
        
        /**
        *   If *max_threads* is not specified,
        *   the pool size is set to the max number
        *   of threads supported by the architecture
        */
        ThreadPool(unsigned int max_threads = std::thread::hardware_concurrency()) : 
            _max_threads(max_threads), 
            _run_pool_thread(true),
            _sem_count(1)
        {
            _create_pool();
        }; 

        /**
        *   Copy constructor
        */ 
        ThreadPool(const ThreadPool &TP) : _mutex() {};
    
        ~ThreadPool() {
            _run_pool_thread = false;
            _sem_wait();
            for (auto &t : _pool) t.detach(); 
            std::cout << "Dealloc" << std::endl;
        };

        /**
        *   Update new size for the thread pool [TODO]
        */
        ThreadPool&
        resize_thread_pool(unsigned int max_threads = std::thread::hardware_concurrency()) {
            assert("Function is not implemented yet");
            return *this;
        }

        /**
        *   Returning the current size of the 
        *   thread pool
        */
        int 
        pool_size() { 
            return _max_threads; 
        }

        int 
        queue_size() {
            return _queue.size();
        } 

        bool 
        queue_is_empty() {
            return _queue.empty();
        }

        /**
        *   Stop execution
        */ 
        void
        stop(bool wait = false) {
            if (!_run_pool_thread) return;
            if (wait) {
                while(!_queue.empty()) {
                    std::this_thread::sleep_for(std::chrono::nanoseconds(1));
                }
                return;
            }
            // TODO: false
            assert("Function is not implemented yet, use wait = true");
        }
    
        /**
        *   Push a job to do in the thread pool
        */
        ThreadPool&
        push(std::function<T(K)> f, K v) {
            _secure_queue_push(std::make_pair(f, v));
            return *this;
        }

        // UNDER DEVELOP
        template<typename ... Args> void
        variadic_push (std::function<T(K)> first, Args ... args) {
            assert("Function is not implemented yet");
        }

        /**
        *   Clear the jobs queue
        */
        void 
        clear() { 
            _sem_wait();
            std::queue<std::pair<std::function<T(K)>, K> > empty;
            std::swap(_queue, empty);
            _sem_signal();
        }
    
    private:
        std::atomic<int> _max_threads;
        std::atomic<int> _sem_count;
        std::atomic<bool> _run_pool_thread;
        std::vector<std::thread> _pool;
        std::queue<std::pair<std::function<T(K)>, K> > _queue;
        std::condition_variable _cv;
        std::mutex _mutex;

        template<class N> inline void 
        _secure_queue_push(N n) {
            _sem_wait();
            _queue.push(n);
            _sem_signal();
        }

        void 
        _create_pool() {
            _pool = std::vector<std::thread> (_max_threads);
            for (int i = 0; i < _max_threads; i++) {
                _pool[i] = std::thread(&ThreadPool::_thread_loop_mth, this);
            }
        }
        
        void 
        _thread_loop_mth() {
            while(_run_pool_thread) {
                _sem_wait();
                if (_queue.empty()) { 
                    _sem_signal();
                    continue; 
                }
                //std::cout << "Pool running thread" << std::this_thread::get_id() << std::endl;
                auto f = _queue.front();
                _queue.pop();
                _sem_signal();
                f.first(f.second);
                //std::cout << "Pool end thread" << std::this_thread::get_id() << std::endl;
            }
        }

        inline void
        _sem_wait() {
            std::unique_lock<std::mutex> lock(this->_mutex);
            while(!_sem_count) _cv.wait(lock);
            _sem_count--;
        }

        inline void 
        _sem_signal() {
            std::unique_lock<std::mutex> lock(this->_mutex);
            _sem_count++;
            _cv.notify_one();
        }

    };

}; // Namespace end

#endif // __cplusplus
#endif // _THREAD_POOL_HPP_
