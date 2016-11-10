#include "test.hpp"
#include "example.hpp"


int 
main() {
    /** Examples */
    example_global_func(10);
    example_inline_code(10);
    example_member_function(2, 10000);
    example_simulate_multithreading_access(20);
    

    /** Tests */
    ThreadPoolTest test = ThreadPoolTest();
    test.exc_all();

    return 0;
}

