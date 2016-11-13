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


namespace astp {

    /**
    *   ThreadPool
    */
    class ThreadPool
    {
    public:

        /**
        *   Internal ThreadPool class 
        *   that represents a binary semaphore
        *   in order to make the ThreadPool thread safe.
        */
        class Semaphore 
        {
        public:
            Semaphore(bool sem_count = true) : sem_value_(sem_count) {};
            Semaphore(const Semaphore &S) : mutex_(), cv_() {};
            Semaphore& operator=(Semaphore S) { return *this; }
            ~Semaphore() {};
    
            inline void
            wait() {
                std::unique_lock<std::mutex> lock(mutex_);
                while(sem_value_ != true) cv_.wait(lock);
                sem_value_ = false;
            }
    
            inline void 
            signal() {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.notify_one();
                sem_value_ = true;
            }
    
        private:
            std::condition_variable cv_;
            std::mutex mutex_;
            std::atomic<bool> sem_value_;
        };

        /**
        *   If *max_threads* is not specified,
        *   the pool size is set to the max number
        *   of threads supported by the architecture.
        *   At least one thread is created.
        */
        ThreadPool(int max_threads = std::thread::hardware_concurrency()) : 
            _max_threads(0),
            _thread_terminate_count(0), 
            _queue_size(0),
            _thread_sleep_time_ns(100000000),
            _run_pool_thread(true),
            _push_c(0),
            _comp_c(0)
        {
            _sem_api = Semaphore(0);
            _sem_job_ins_container = Semaphore(0);
            resize(max_threads < 1 ? 1 : max_threads);
        }; 

        /**
        *   Copy constructor.
        */ 
        ThreadPool(const ThreadPool &TP) {};
    
        /**
        *   When the ThreadPool is deallocated,
        *   the threads still running are joined().
        */
        ~ThreadPool() {
            if (_run_pool_thread) {
                _run_pool_thread = false;
                for (auto &t : _pool) t.join();
            }
        };

        /**
        *   Update size for the thread pool;
        *   the abs value of num_threads is taken.
        */
        ThreadPool&
        resize(int num_threads = std::thread::hardware_concurrency()) noexcept {
            _sem_api.wait();
            if (num_threads < 1) { num_threads = 1; }
            int diff = abs(num_threads - (int)_max_threads);
            if (num_threads > _max_threads) {
                for (int i = 0; i < diff; i++) _pool_push_thread();
            } else {
                for (int i = 0; i < diff; i++) _pool_pop_thread();
            }
            _sem_api.signal();
            return *this;
        }

        /**
        *   Push a job to do in jobs queue.
        *   Use lambda expressions in order to
        *   load jobs.
        */
        template<class F> void
        push(const F &f) noexcept {
            _push_c++;
            _safe_queue_push(f);
        }

        ThreadPool&
        synchronize() {
            _sem_job_ins_container.wait();
            return *this;
        }

        ThreadPool&
        end_synchronize() {
            _sem_job_ins_container.signal();
            return *this;
        }

        /**
        *   Stop execution, detach all
        *   jobs under processing.
        */ 
        ThreadPool&
        stop() noexcept {
            if (!_run_pool_thread) return *this;
            _sem_api.wait();
            int current_threads_num = _max_threads;
            _run_pool_thread = false;
            while(_max_threads != 0) 
                _pool_pop_thread();
            while(current_threads_num != _thread_terminate_count) 
                std::this_thread::yield();
            _thread_terminate_count = 0;
            _sem_api.signal();
            return *this;
        }

        /**
        *   Wait until all jobs
        *   are computed.
        */
        ThreadPool&
        wait() noexcept {
            if (!_run_pool_thread) return *this;
            while((_push_c != _comp_c)) 
                std::this_thread::sleep_for(std::chrono::nanoseconds(_thread_sleep_time_ns));
            _push_c = 0;
            _comp_c = 0;
            return *this;
        }

        /**
        *   Returning the current size of the 
        *   thread pool.
        */
        int 
        pool_size() noexcept { 
            return _max_threads; 
        }

        size_t
        queue_size() {
            return _queue_size;
        }

        bool 
        queue_is_empty() noexcept {
            return _queue_size == 0;
        }

        /**
        *   Set the thread sleep time.
        *   Interval is in nanoseconds.
        */
        ThreadPool&
        set_sleep_time_ns(int time_ns) noexcept {
            _thread_sleep_time_ns = abs(time_ns);
            return *this;
        }

        int 
        sleep_time_ns() noexcept {
            return _thread_sleep_time_ns;
        }
    
    private:
        /** 
        *   Mutex for queue access. 
        */
        std::mutex _mutex_queue;

        /** 
        *   Mutex for pool resize. 
        */
        std::mutex _mutex_pool;
        
        /** 
        *   Semaphore for class thread-safety. 
        */
        Semaphore _sem_api;

        /**
        *   Optional semaphore for jobs lambda data
        *   protection in critical sections.
        */
        Semaphore _sem_job_ins_container;

        /** 
        *   Time in nanoseconds which threads
        *   that are sleeping check for new
        *   jobs in the queue.
        */
        std::atomic<int> _thread_sleep_time_ns;
        
        /**
        *   Flag for pool's threads state,
        *   when false, all the threads will be
        *   detached.
        */
        std::atomic<bool> _run_pool_thread;

        std::atomic<int> _max_threads;
        std::atomic<size_t> _queue_size;
        std::vector<std::thread> _pool;
        std::queue<std::function<void()> > _queue;
        std::vector<std::thread::id> _thread_to_terminate;
        std::atomic<int> _thread_terminate_count;
        
        std::atomic<int> _push_c;
        std::atomic<int> _comp_c;


        /**
        *   Lock the queue mutex for
        *   a safe insertion in the queue.
        */
        template<class F> inline void
        _safe_queue_push(const F t) {
            std::unique_lock<std::mutex> lock(_mutex_queue);
            _queue.push(t);
            _queue_size++;
        }

        /**
        *   Lock the queue mutex, safely pop
        *   job from the queue if not empty;
        *   returns a pair where the first value
        *   is false if the queue is empty, the 
        *   second value is the lamda expression
        *   to execute.
        */
        inline std::pair<bool, std::function<void()> >
        _safe_queue_pop() {
            std::unique_lock<std::mutex> lock(_mutex_queue);
            if (_queue.empty()) return std::make_pair(false, std::function<void()>()); 
            auto t = _queue.front();
            _queue.pop();
            _queue_size--;
            return std::make_pair(true, t);
        }

        /**
        *   Called when the ThreadPool is created 
        *   or the user has required a resize 
        *   operation.
        */
        inline void 
        _pool_push_thread() {
            _pool.push_back(std::thread(&ThreadPool::_thread_loop_mth, this));
            _max_threads++;
        }

        /**
        *   Called when the ThreadPool is deleted 
        *   or the user has required both a resize 
        *   operation or a stop operation.
        */
        inline void 
        _pool_pop_thread() {
            std::unique_lock<std::mutex> lock(_mutex_pool);
            if (_pool.empty()) return; 
            _thread_to_terminate.push_back(_pool.back().get_id());
            _pool.back().detach();
            _pool.pop_back();
            _max_threads--;
        }

        inline bool
        _pool_check_terminate_thread(std::thread::id thread_id) {
            std::unique_lock<std::mutex> lock(_mutex_pool);
            for (int i = 0; i < _thread_to_terminate.size(); i++) {
                if (std::this_thread::get_id() == _thread_to_terminate[i]) {
                    _thread_to_terminate.erase(_thread_to_terminate.begin() + i);
                    return true;
                }
            }
            return false;
        }

        /**
        *   Each thread start run this function
        *   when the thread is created, and 
        *   exit only when the pool is destructed
        *   or the stop() function is called.
        *   The thread go to sleep if the 
        *   queue is empty. 
        */
        void 
        _thread_loop_mth() noexcept {
            while(_run_pool_thread) {
                if (_pool_check_terminate_thread(std::this_thread::get_id())) break;
                auto funcf = _safe_queue_pop();
                if (!funcf.first) { 
                    // Sleep
                    std::this_thread::sleep_for(std::chrono::nanoseconds(_thread_sleep_time_ns));
                    continue; 
                }
                try {
                    funcf.second();
                    _comp_c++;  
                } catch (...) {
                    // TODO
                }
            }
            _thread_terminate_count++;
        }

    }; // End ThreadPool

}; // Namespace end

#endif // __cplusplus
#endif // _THREAD_POOL_HPP_

