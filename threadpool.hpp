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


    class ThreadPool
    {
    public:
        /**
        *   If *max_threads* is not specified,
        *   the pool size is set to the max number
        *   of threads supported by the architecture.
        *   At least one thread is created.
        */
        ThreadPool(int max_threads = std::thread::hardware_concurrency()) : 
            _max_threads(0), 
            _queue_size(0),
            _thread_sleep_time_ns(100000000),
            _run_pool_thread(true)
        {
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
        resize(int num_threads = std::thread::hardware_concurrency()) {
            std::unique_lock<std::mutex> lock(_mutex_pool);
            if (num_threads < 1) { num_threads = 1; }
            int diff = abs(num_threads - (int)_max_threads);
            if (num_threads > _max_threads) {
                //_pool.reserve(_pool.size() + diff); TOCHECK
                for (int i = 0; i < diff; i++) _pool_push_thread();
            } else {
                for (int i = 0; i < diff; i++) _pool_pop_thread();
            }
            return *this;
        }

        /**
        *   Convenience method.
        *   Adapt the pool size to the jobs queue size.
        */
        ThreadPool&
        autofit() {
            return resize(_queue_size);
        }

        /**
        *   Returning the current size of the 
        *   thread pool.
        */
        int 
        pool_size() { 
            return _max_threads; 
        }

        size_t
        queue_size() {
            return _queue_size;
        }

        bool 
        queue_is_empty() {
            return _queue_size == 0;
        }

        /**
        *   Stop execution, detach all
        *   jobs under processing.
        */ 
        void
        stop() {
            if (!_run_pool_thread) return;
            _run_pool_thread = false;
            while(_max_threads != 0) {
                _pool_pop_thread();
            }
        }

        /**
        *   Wait until all jobs
        *   are computed.
        */
        void
        wait() {
            if (!_run_pool_thread) return;
            while(!queue_is_empty()) {
                std::this_thread::sleep_for(std::chrono::nanoseconds(100));
            }
            return;
        }

        /**
        *   Set the thread sleep time.
        *   Interval is in nanoseconds.
        */
        ThreadPool&
        set_sleep_time_ns(int time_ns) {
            _thread_sleep_time_ns = abs(time_ns);
            return *this;
        }

        int 
        sleep_time_ns() {
            return _thread_sleep_time_ns;
        }

        /**
        *   Push a job to do in jobs queue.
        *   Use lamda expressions in order to
        *   load jobs.
        */
        template<class F> void
        push(const F &f) {
            _safe_queue_push(f);
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
        void 
        _pool_push_thread() {
            _pool.push_back(std::thread(&ThreadPool::_thread_loop_mth, this));
            _max_threads++;
        }


        /**
        *   Called when the ThreadPool is deleted 
        *   or the user has required both a resize 
        *   operation or a stop operation.
        */
        void 
        _pool_pop_thread() {
            _thread_to_terminate.push_back(_pool.back().get_id());
            _pool.back().detach();
            _pool.pop_back();
            _max_threads--;
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
                for (int i = 0; i < _thread_to_terminate.size(); i++) {
                    if (std::this_thread::get_id() == _thread_to_terminate[i]) {
                        // Exit and terminate thread
                        _thread_to_terminate.erase(_thread_to_terminate.begin() + i);
                        return;
                    }
                }
                auto funcf = _safe_queue_pop();
                if (!funcf.first) { 
                    // Sleep
                    std::this_thread::sleep_for(std::chrono::nanoseconds(_thread_sleep_time_ns));
                    continue; 
                }
                try {
                    funcf.second();  
                } catch (...) {
                    // TODO
                }
            }
        }

    }; // End ThreadPool

}; // Namespace end

#endif // __cplusplus
#endif // _THREAD_POOL_HPP_

