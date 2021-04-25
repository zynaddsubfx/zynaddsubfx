#include "common.h"

#define TS_ASSERT(b) \
    assert_true(b, #b, __LINE__)

#define TS_ASSERT_DELTA(a,b,t) \
    assert_f32_sim(a,b,t,"similar floats", __LINE__)

#define TS_ASSERT_EQUAL_STR(a,b) \
    assert_str_eq(a,b,a " == " #b, __LINE__)

#define TS_ASSERT_EQUAL_CPP(a,b) \
    assert_true(a == b,"equality check", __LINE__)

#define TS_ASSERT_EQUAL_INT(a,b) \
    assert_int_eq(a,b,"similar ints", __LINE__)

#define TS_ASSERT_EQUAL_FLT(a,b) \
    assert_f32_eq(a,b,"similar ints", __LINE__)
#define TS_NON_NULL(a) \
    assert_non_null(a, "valid pointer", __LINE__)

#define RUN_TEST(x) \
    test.setUp();\
    test.x();\
    test.tearDown()
