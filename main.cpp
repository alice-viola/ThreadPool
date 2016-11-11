#include "test.hpp"
#include "example.hpp"


int 
main() {
    /** Examples */
    example_inline_code_and_save(7);
    example_global_func(10);
    example_inline_code(10);
    example_member_function(2, 10000);
    example_inline_code_and_save_vec(1000);

    /** Tests */
    ThreadPoolTest test = ThreadPoolTest();
    test.exc_all();

    return 0;
}

