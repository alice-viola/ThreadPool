#ifndef _THREAD_POOL_TEST_HPP_
#define _THREAD_POOL_TEST_HPP_
#ifdef __cplusplus

#include <iostream>
#include <stdlib.h>     
#include <time.h>  
#include "cppunit/TestCase.h"
#include "cppunit/TestCaller.h"
#include "cppunit/TestResult.h"
#include "cppunit/TestSuite.h"
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include "threadpool.hpp"

using namespace astp;

class ThreadPoolTest: public CppUnit::TestFixture  
{
public:
    
    void 
    setUp() {
        srand(time(NULL));
        tp = new ThreadPool(4);
    }
    
    void 
    tearDown() {
        delete tp;
    }

    void
    testResize() {
        try {
            int r = random(-10,10);
            tp->resize(r);
            CPPUNIT_ASSERT( tp->pool_size() == r );
        } catch(std::runtime_error e) {}
    }

    void
    testStop() {
        tp->stop();
        CPPUNIT_ASSERT( tp->pool_size() == 0 );
    }

    void
    testAwake() {
        int s = tp->pool_size();
        tp->stop();
        tp->awake();
        CPPUNIT_ASSERT( tp->pool_size() == s );
    }

    void
    testPush() {
        tp->stop();
        int r = random(1,1000);
        for (int i = 0; i < r; i++) {
            tp->push([](){});
        }
        CPPUNIT_ASSERT( tp->queue_size() == r );
    }

    void
    testVariadicPush() {
        tp->stop();
        int r = random(1,1000);
        for (int i = 0; i < r; i++) {
            tp->push([](){}, [](){}, [](){});
        }
        CPPUNIT_ASSERT( tp->queue_size() == (r * 3) );
    }

    void
    testPushOperator() {
        tp->stop();
        int r = random(1,1000);
        for (int i = 0; i < r; i++) {
            *tp << [](){};
        }
        CPPUNIT_ASSERT( tp->queue_size() == r );
    }

    void
    testApplyFor() {
        try {
            auto it = random(-100, 1000);
            tp->apply_for(it, [](){});
            CPPUNIT_ASSERT( tp->queue_size() == 0 );
        } catch(std::runtime_error e) {}
    }

    void
    testApplyForAsync() {
        try {
            auto it = random(-100, 1000);
            tp->apply_for_async(it, [](){});
            tp->wait();
            CPPUNIT_ASSERT( tp->queue_size() == 0 );
        } catch(std::runtime_error e) {}
    }

    void
    testFuture() {
        int r = random(-1000, 1000);
        auto fut = tp->future_from_push([r]() -> int { return r; });
        fut.wait();
        CPPUNIT_ASSERT( fut.get() == r );
    }

    void
    testSleepTime() {
        try {
            int v = random(-1000,1000);
            tp->set_sleep_time_ns(v);
            CPPUNIT_ASSERT( tp->sleep_time_ns() == v ); 
            tp->set_sleep_time_ms(v);
            CPPUNIT_ASSERT( tp->sleep_time_ns() == (v * 1000000) );   
            tp->set_sleep_time_ns(v);
            CPPUNIT_ASSERT( tp->sleep_time_ns() == (v * 1000000000) );         
        } catch (std::runtime_error e) {}
    }

    void 
    testDispatchGroupOpen() {
        try {
            tp->dg_open("t1");
        } catch (std::runtime_error e) {
            CPPUNIT_ASSERT( false ); 
        }
    }

    void 
    testDispatchGroupClose() {
        try {
            tp->dg_open("t1");
            tp->dg_close("t1");
        } catch (std::runtime_error e) {
            CPPUNIT_ASSERT( false ); 
        }
    }

    void 
    testDispatchGroupInsert() {
        try {
            tp->stop();
            tp->dg_open("t1");
            int r = random(1, 1000);
            for (int i = 0; i < r; i++) {
                tp->dg_insert("t1",[](){});    
            }
            tp->dg_close("t1");
            CPPUNIT_ASSERT( tp->queue_size() == r ); 
        } catch (std::runtime_error e) {
            CPPUNIT_ASSERT( false ); 
        }
    }

    void 
    testDispatchGroupWrongOpen() {
        try {
            tp->dg_open("t1");
            tp->dg_open("t1");
            CPPUNIT_ASSERT( false );
        } catch (std::runtime_error e) {
            CPPUNIT_ASSERT( true ); 
        }
    }

    void 
    testDispatchGroupWrongClose() {
        try {
            tp->dg_close("t1");
            CPPUNIT_ASSERT( false );
        } catch (std::runtime_error e) {
            CPPUNIT_ASSERT( true ); 
        }
    }

    void 
    testDispatchGroupWrongInsert() {
        try {
            tp->stop();
            tp->dg_insert("t1",[](){});    
            tp->dg_close("t1");
            CPPUNIT_ASSERT( false); 
        } catch (std::runtime_error e) {
            CPPUNIT_ASSERT( true ); 
        }
    }

    void 
    testDispatchGroupWaitAndFire() {
        try {
            tp->dg_open("t1");
            tp->dg_insert("t1",[](){});    
            tp->dg_close("t1");
            int a = 0;
            tp->dg_wait("t1", [&a](){ a = 30; });  
            CPPUNIT_ASSERT( a == 30);
            CPPUNIT_ASSERT( tp->queue_size() == 0); 
        } catch (std::runtime_error e) {
            CPPUNIT_ASSERT( false ); 
        }
    }

    void 
    testDispatchGroupNow() {
        try {
            tp->dg_now("t1",[](){});  
            tp->dg_wait("t1");  
            CPPUNIT_ASSERT( tp->queue_size() == 0); 
        } catch (std::runtime_error e) {
            CPPUNIT_ASSERT( false ); 
        }
    }

    void 
    testDispatchGroupCloseBarrier() {
        try {
            int a = 0;
            tp->dg_open("t1");
            tp->dg_insert("t1", [](){});
            tp->dg_close_with_barrier("t1", [&a](){ a = 30; });
            tp->dg_wait("t1");
            CPPUNIT_ASSERT( a == 30);
            CPPUNIT_ASSERT( tp->queue_size() == 0); 
        } catch (std::runtime_error e) {
            CPPUNIT_ASSERT( false ); 
        }
    }

    void
    testSetExcHandl() {
        std::string err;
        std::string oerr = "ERR1";
        std::function<void(std::string)> efunc = [&err](std::string e) { 
            err = e;
        };
        tp->set_excpetion_action(efunc);
        try {
            *tp << [oerr](){ throw oerr; };
            tp->wait();
            CPPUNIT_ASSERT( true ); 
        } catch (std::string e) {
            CPPUNIT_ASSERT( e == oerr ); 
        }
    }


private:
    CPPUNIT_TEST_SUITE(ThreadPoolTest);
    CPPUNIT_TEST(testResize);
    CPPUNIT_TEST(testStop);
    CPPUNIT_TEST(testAwake);
    CPPUNIT_TEST(testPush);
    CPPUNIT_TEST(testVariadicPush);
    CPPUNIT_TEST(testPushOperator);
    CPPUNIT_TEST(testApplyFor);
    CPPUNIT_TEST(testApplyForAsync);
    CPPUNIT_TEST(testFuture);
    CPPUNIT_TEST(testSleepTime);
    CPPUNIT_TEST(testDispatchGroupOpen);
    CPPUNIT_TEST(testDispatchGroupClose);
    CPPUNIT_TEST(testDispatchGroupInsert);
    CPPUNIT_TEST(testDispatchGroupWrongOpen);
    CPPUNIT_TEST(testDispatchGroupWrongClose);
    CPPUNIT_TEST(testDispatchGroupWrongInsert);
    CPPUNIT_TEST(testDispatchGroupWaitAndFire);
    CPPUNIT_TEST(testDispatchGroupNow);
    CPPUNIT_TEST(testDispatchGroupCloseBarrier);
    CPPUNIT_TEST(testSetExcHandl);
    CPPUNIT_TEST_SUITE_END();

    ThreadPool *tp;

    int
    random(int min, int max) {
        return rand() % max + min;
    }
};


void 
test_threadpool() {
    CppUnit::TextUi::TestRunner runner;
    runner.addTest(ThreadPoolTest::suite());
    runner.run();
}

#endif // __cplusplus
#endif // _THREAD_POOL_TEST_HPP_
