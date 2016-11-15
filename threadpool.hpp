/**
*
*   The MIT License (MIT)
*   
*   Copyright (c) 2016 Amedeo Setti
*   
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*   
*   The above copyright notice and this permission notice shall be included in all
*   copies or substantial portions of the Software.
*   
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NON INFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE
*
**/

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
#include <string>
#include <queue>
#include <chrono>
#include <assert.h>

namespace astp {
    /**
    *    _____ _                        _ ____             _ 
    *   |_   _| |__  _ __ ___  __ _  __| |  _ \ ___   ___ | |
    *     | | | '_ \| '__/ _ \/ _` |/ _` | |_) / _ \ / _ \| |
    *     | | | | | | | |  __/ (_| | (_| |  __/ (_) | (_) | |
    *     |_| |_| |_|_|  \___|\__,_|\__,_|_|   \___/ \___/|_|
    *
    *     BECAUSE POWER IS NOTHING WITHOUT CONTROL                                                          
    */
    class ThreadPool
    {
    protected:
        /**
        *    ____                             _                    
        *   / ___|  ___ _ __ ___   __ _ _ __ | |__   ___  _ __ ___ 
        *   \___ \ / _ \ '_ ` _ \ / _` | '_ \| '_ \ / _ \| '__/ _ \
        *    ___) |  __/ | | | | | (_| | |_) | | | | (_) | | |  __/
        *   |____/ \___|_| |_| |_|\__,_| .__/|_| |_|\___/|_|  \___|
        *                      |_|                         
        *
        *
        *   Internal ThreadPool class 
        *   that represents a binary semaphore
        *   in order to make the ThreadPool thread safe.
        */
        class Semaphore 
        {
        public:
            Semaphore(bool sem_count = true) : _sem_value(sem_count) {};
            Semaphore(const Semaphore &S) : _mutex(), _cv() {};
            Semaphore& operator=(Semaphore S) { return *this; }
            ~Semaphore() {};
    
            inline void
            wait() {
                std::unique_lock<std::mutex> lock(_mutex);
                while(_sem_value != true) _cv.wait(lock);
                _sem_value = false;
            }
    
            inline void 
            signal() {
                std::unique_lock<std::mutex> lock(_mutex);
                _cv.notify_one();
                _sem_value = true;
            }
    
        private:
            std::condition_variable _cv;
            std::mutex _mutex;
            std::atomic<bool> _sem_value;
        };

        /**
        *    ____  _                 _       _      ____                       
        *   |  _ \(_)___ _ __   __ _| |_ ___| |__  / ___|_ __ ___  _   _ _ __  
        *   | | | | / __| '_ \ / _` | __/ __| '_ \| |  _| '__/ _ \| | | | '_ \ 
        *   | |_| | \__ \ |_) | (_| | || (__| | | | |_| | | | (_) | |_| | |_) |
        *   |____/|_|___/ .__/ \__,_|\__\___|_| |_|\____|_|  \___/ \__,_| .__/ 
        *               |_|                                             |_|    
        *   
        *
        *   Internal ThreadPool class 
        *   that stores informations
        *   about dispatch groups.
        */
        class DispatchGroup
        {
        public:
            DispatchGroup(std::string id) : 
                _id(id), 
                _closed(false),
                _jobs_done_counter(0),
                _jobs_count_at_leave(0) {};
            DispatchGroup(const DispatchGroup &DP) :
                _id(DP.id()), 
                _closed(DP.is_leave()),
                _jobs_done_counter(0),
                _jobs_count_at_leave(0) {};
            ~DispatchGroup() {};

            inline void 
            leave() noexcept {
                _closed = true;
                _jobs_count_at_leave = _jobs.size();
            }

            inline bool
            is_leave() const noexcept { return _closed; }

            template<class F> inline void
            insert(const F &f) noexcept {
                if (_closed) return;
                auto func = [=] () { f(); _signal_end_of_job(); };
                _jobs.push_back(func);
            }

            inline std::vector<std::function<void()> > 
            jobs() noexcept { return _jobs; }

            inline bool
            has_finished() noexcept {
                if (_jobs_done_counter == _jobs_count_at_leave && _closed) return true;
                return false;
            }

            inline std::string
            id() const noexcept { return _id; }

            inline int
            jobs_count() noexcept { return _jobs.size(); }

            inline void
            synchronize() {
                _sem_sync.wait();
            }

            inline void
            end_synchronize() {
                _sem_sync.signal();
            }
            
        private:
            std::string _id;
            std::vector<std::function<void()> > _jobs;
            std::atomic<bool> _closed;
            std::atomic<int> _jobs_count_at_leave;
            std::atomic<int> _jobs_done_counter;
            Semaphore _sem_sync;

            inline void
            _signal_end_of_job() { _jobs_done_counter++; }
        };

    public:

        /**
        *   If *max_threads* is not specified,
        *   the pool size is set to the max number
        *   of threads supported by the architecture.
        *   At least one thread is created.
        */
        ThreadPool(int max_threads = std::thread::hardware_concurrency()) : 
            _max_threads(0),
            _thread_to_kill_c(),
            _queue_size(0),
            _thread_sleep_time_ns(1000),
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
                for (int i = 0; i < diff; i++) _safe_thread_push();
            } else {
                for (int i = 0; i < diff; i++) _safe_thread_pop();
            }
            _sem_api.signal();
            return *this;
        }

        /**
        *   Push a job to do in jobs queue.
        *   Use lambda expressions in order to
        *   load jobs.
        */
        template<class F> inline ThreadPool&
        push(const F &f) noexcept {
            _push_c++;
            _safe_queue_push(f);
            return *this;
        }

        /**
        *   Push multiple jobs to do in jobs queue.
        *   Use lambda expressions in order to
        *   load jobs.
        */
        template<class F, class ...Args> inline ThreadPool&
        push(const F &f, Args... args) noexcept {
            push(f).push(args...);
            return *this;
        }

        inline ThreadPool&
        synchronize() noexcept {
            _sem_job_ins_container.wait();
            return *this;
        }

        inline ThreadPool&
        end_synchronize() noexcept {
            _sem_job_ins_container.signal();
            return *this;
        }

        /**
        *   Stop execution, detach all
        *   jobs under processing.
        *   This is a thread blocking call.
        */ 
        ThreadPool&
        stop() noexcept {
            if (!_run_pool_thread) return *this;
            _sem_api.wait();
            int current_threads_num = _max_threads;
            _run_pool_thread = false;
            while(_max_threads != 0) _safe_thread_pop();
            while(_thread_to_kill_c != 0) {
                std::this_thread::sleep_for(std::chrono::nanoseconds(_thread_sleep_time_ns));
            }
            _sem_api.signal();
            return *this;
        }

        /**
        *   Wait until all jobs
        *   are computed.
        *   This is a thread blocking call.
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
        queue_size() noexcept {
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
        set_sleep_time_ns(const int time_ns) noexcept {
            _thread_sleep_time_ns = abs(time_ns);
            return *this;
        }

        /**
        *   Set the thread sleep time.
        *   Interval is in milliseconds.
        */
        ThreadPool&
        set_sleep_time_ms(const int time_ms) noexcept {
            _thread_sleep_time_ns = abs(time_ms * 1000000);
            return *this;
        }

        /**
        *   Set the thread sleep time.
        *   Interval is in seconds
        *   and can be a floating point value.
        */
        template<class F> ThreadPool&
        set_sleep_time_s(F time_s) noexcept {
            _thread_sleep_time_ns = abs(static_cast<int>(time_s * 1000000000));
            return *this;
        }

        int 
        sleep_time_ns() noexcept {
            return _thread_sleep_time_ns;
        }

        /**
        *    ____   ____                           
        *   |  _ \ / ___|_ __ ___  _   _ _ __  ___ 
        *   | | | | |  _| '__/ _ \| | | | '_ \/ __|
        *   | |_| | |_| | | | (_) | |_| | |_) \__ \
        *   |____/ \____|_|  \___/ \__,_| .__/|___/
        *                               |_|        
        *
        *   Set of functions for command dispatch_group
        *   operations.
        *
        *
        *   Create a new group with an std::string 
        *   identifier.
        */
        ThreadPool&
        dispatch_group_enter(const std::string id) noexcept {
            std::unique_lock<std::mutex> lock(_mutex_groups);
            std::map<std::string, DispatchGroup>::iterator it;
            it = _groups.find(id);
            if (it != _groups.end()) return *this;
            _groups.insert(std::make_pair(id, DispatchGroup(id)));
            return *this;
        }
        /**
        *   Insert a job to do in a specific group.
        *   If the group not exist, nothing is done.
        *   Task will not start until a call to 
        *   leave will be done.
        */
        template<class F> inline ThreadPool&
        dispatch_group_insert(const std::string id, const F &f) noexcept {
            std::unique_lock<std::mutex> lock(_mutex_groups);
            for (std::map<std::string, DispatchGroup>::iterator it=_groups.begin(); it!=_groups.end(); ++it) {
                if (it->first == id) { it->second.insert(f); break; }
            }
            return *this;
        }
        /**
        *   Signal to a group that the jobs immission 
        *   is end, than start pushing the group jobs
        *   to the standard threadpool queue.
        */
        ThreadPool& 
        dispatch_group_leave(const std::string id) noexcept {
            std::unique_lock<std::mutex> lock(_mutex_groups);
            for (std::map<std::string, DispatchGroup>::iterator it=_groups.begin(); it!=_groups.end(); ++it) {
                if (it->first != id) continue;
                it->second.leave();
                auto jobs = it->second.jobs();
                for (auto &j : jobs) { push(j); }
                break;
            }
            return *this;
        }
        /**
        *   Wait until every job in a group is computed.
        *   This is a thread blocking call.
        */
        ThreadPool& 
        dispatch_group_wait(const std::string id) noexcept {
            for (std::map<std::string,DispatchGroup>::iterator it=_groups.begin(); it!=_groups.end(); ++it) {
                if (it->first != id) continue;
                while(!it->second.has_finished()) std::chrono::nanoseconds(_thread_sleep_time_ns); 
                break;
            }
            return *this;
        }
        /** 
        *   As the normal dispatch_group_wait, but
        *   at the end call the function f.
        *   Usefull if you want to signal somewhat
        *   at the end of group.
        */
        template<class F> ThreadPool&  
        dispatch_group_wait(const std::string id, const F &f) {
            dispatch_group_wait(id); f();
            return *this;
        }
        /**
        *   The same as synchronize, but is usefull
        *   if you don't want do block all others
        *   jobs in the queue.
        */
        ThreadPool& 
        dispatch_group_synchronize(const std::string id) noexcept {
            for (std::map<std::string,DispatchGroup>::iterator it=_groups.begin(); it!=_groups.end(); ++it) {
                if (it->first != id) continue;
                it->second.synchronize();
            }
            return *this;
        }
        /**/
        ThreadPool& 
        dispatch_group_end_synchronize(const std::string id) noexcept {
            for (std::map<std::string,DispatchGroup>::iterator it=_groups.begin(); it!=_groups.end(); ++it) {
                if (it->first != id) continue;
                it->second.end_synchronize();
            }
            return *this;
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
        *   Mutex for groups access. 
        */
        std::mutex _mutex_groups;
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
        std::atomic<size_t> _queue_size;
        /** 
        *   Where the running threads lives. 
        */
        std::vector<std::thread> _pool;
        /** 
        *   Queue of jobs to do.
        */
        std::queue<std::function<void()> > _queue;
        /** 
        *   A map of in process groups of jobs.
        */
        std::map<std::string, DispatchGroup> _groups;
        /** 
        *   The number of threads currently in the pool.
        */
        std::atomic<int> _max_threads;
        /** 
        *   Counter used when there are 
        *   some threads to remove from
        *   the pool [stop or resize]. 
        */
        std::atomic<int> _thread_to_kill_c;
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
        *   job from the queue if not empty.
        */
        inline std::function<void()>
        _safe_queue_pop() {
            std::unique_lock<std::mutex> lock(_mutex_queue);
            if (_queue.empty()) return std::function<void()>(); 
            auto t = _queue.front();
            _queue.pop();
            _queue_size--;
            return t;
        }

        /**
        *   Called when the ThreadPool is created 
        *   or the user has required a resize 
        *   operation.
        */
        inline void 
        _safe_thread_push() {
            _pool.push_back(std::thread(&ThreadPool::_thread_loop_mth, this));
            _max_threads++;
        }

        /**
        *   Called when the ThreadPool is deleted 
        *   or the user has required both a resize 
        *   operation or a stop operation.
        */
        inline void 
        _safe_thread_pop() {
            std::unique_lock<std::mutex> lock(_mutex_pool);
            if (_pool.empty()) return; 
            _thread_to_kill_c++;
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
                if (_thread_to_kill_c != 0) break;
                auto funcf = _safe_queue_pop();
                if (!funcf) { 
                    if (_thread_sleep_time_ns != 0) 
                        std::this_thread::sleep_for(std::chrono::nanoseconds(_thread_sleep_time_ns));
                    continue; 
                }
                try {
                    funcf();
                    _comp_c++;  
                } catch (...) {
                    // TODO
                }
            }
            _thread_to_kill_c--;
        }

    }; // End ThreadPool

}; // Namespace end

#endif // __cplusplus
#endif // _THREAD_POOL_HPP_

