#include "StringView.h"
#include "TestUtils.h"

#define TEST_SUITE(name)                                                       \
    static const char *test_suite_name = name;                                 \
    static int test_suite_count = 0;                                           \
    static int test_suite_failed_count = 0;

#define TEST(name)
static const char *test_name = name;

int main(void) {
    program_epoch_ns = get_current_ns();
    TEST_SUITE("ctors") {
        TEST("ctor 1") {
            StringView t = sv_make("foo");
            sv_dbg_print(t);
            ASSERT(streq("foo", t.ptr));
        }
    }
}
