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
#include <map>
#include <queue>
#include <chrono>
#include <assert.h>
#include <iostream>

namespace astp {

    class Semaphore 
    {
    public:
        Semaphore(int sem_count = 1) : sem_count_(sem_count) {};
        Semaphore(const Semaphore &S) : mutex_(), cv_() {};
        Semaphore& operator=(Semaphore S) {
            return *this;
        }
        ~Semaphore() {};

        void
        wait() {
            std::unique_lock<std::mutex> lock(this->mutex_);
            cv_.wait(lock, [this](){ return sem_count_ != 0; });
            sem_count_--;
        }

        void 
        signal() {
            std::unique_lock<std::mutex> lock(this->mutex_);
            sem_count_++;
            cv_.notify_one();
        }

    private:
        std::condition_variable cv_;
        std::mutex mutex_;
        int sem_count_;
    };


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
            _run_pool_thread(true)
        {
            _create_pool();
            //_semaphore_m["queue"] = Semaphore(1);
        }; 

        /**
        *   Copy constructor
        */ 
        ThreadPool(const ThreadPool &TP) {};
    
        ~ThreadPool() {
            _run_pool_thread = false;
            for (auto &t : _pool) t.join(); 
        };

        /**
        *   Update new size for the thread pool [TODO]
        */
        ThreadPool&
        resize(unsigned int max_threads = std::thread::hardware_concurrency()) {
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
        *   Stop execution, detach all
        *   jobs under processing
        */ 
        void
        stop() {
            assert("Function is not implemented yet, use wait = true");
        }

        /**
        *   Wait until all jobs
        *   are computed
        */
        void
        wait() {
            if (!_run_pool_thread) return;
            while(!_queue_is_empty) {
                std::this_thread::sleep_for(std::chrono::nanoseconds(1));
            }
            return;
        }
    
        /**
        *   Push a job to do in the thread pool
        */
        ThreadPool&
        push(std::function<T(K)> f, K v) {
            _safe_queue_push(std::make_pair(f, v));
            return *this;
        }

        // UNDER DEVELOP
        template<typename ... Args> void
        variadic_push (std::function<T(K)> first, Args ... args) {
            assert("Function is not implemented yet");
        }
    
    private:
        std::atomic<int> _max_threads;
        std::atomic<bool> _run_pool_thread;
        std::vector<std::thread> _pool;
        std::queue<std::pair<std::function<T(K)>, K> > _queue;
        std::map<std::string, Semaphore> _semaphore_m;
        std::mutex mutex;
        std::atomic<bool> _queue_is_empty;


        template<class N> void
        _safe_queue_push(const N t) {
            std::unique_lock<std::mutex> lock(this->mutex);
            _queue_is_empty = false;
            _queue.push(t);
        }

        std::pair<bool, std::pair<std::function<T(K)>, K> >
        _safe_queue_pop() {
            std::unique_lock<std::mutex> lock(this->mutex);
            if (_queue.empty()) { 
                _queue_is_empty = true;
                return std::make_pair(false, std::pair<std::function<T(K)>, K>()); 
            }
            auto t = _queue.front();
            _queue.pop();
            return std::make_pair(true, t);
        }

        bool 
        _safe_queue_empty() {
            std::unique_lock<std::mutex> lock(this->mutex);
            return _queue_is_empty;
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
                auto funcf = _safe_queue_pop();
                if (!funcf.first) { continue; }
                auto res = funcf.second.first(funcf.second.second);    
            }
        }

    };

}; // Namespace end

#endif // __cplusplus
#endif // _THREAD_POOL_HPP_



/*

        void 
        clear() { 
            _semaphore_m["queue"].wait();
            std::queue<std::pair<std::function<T(K)>, K> > empty;
            std::swap(_queue, empty);
            _semaphore_m["queue"].signal();
        }

*/