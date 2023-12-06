#include <stdio.h>

struct test_stats {
    int tests_run;
    int tests_passed;
} test_stats = {0, 0};

#define GREEN   "\033[32m"      /* Green */
#define RED     "\033[31m"      /* Red */

#define RESET   "\033[0m"

void testprintff(int test,  const char* test_str)
{
    if(test){
        fprintf(stderr, "[ " GREEN "PASS" RESET " ] %s\n", test_str);
        test_stats.tests_passed++;
    } else {
        fprintf(stderr, "[ " RED "FAILED" RESET " ] %s\n", test_str);
    }
    test_stats.tests_run++;
}

void test_summary()
{
    fprintf(stderr, "\n\n");
    fprintf(stderr, "Tests run: %d\n", test_stats.tests_run);
    fprintf(stderr, "Tests passed: %d\n", test_stats.tests_passed);
    fprintf(stderr, "Tests failed: %d\n", test_stats.tests_run - test_stats.tests_passed);
    fprintf(stderr, "\n\n");
}