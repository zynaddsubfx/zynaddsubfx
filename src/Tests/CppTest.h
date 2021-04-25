#ifndef MSGPARSETEST_H
#define MSGPARSETEST_H

#include <cstddef>
#include <utility>
#include <cxxtest/TestSuite.h>

template<std::size_t N>
class MyArray
{
    template<std::size_t ... Indices>
    MyArray<N> copy_internal(std::index_sequence<Indices...>) const {
        return MyArray<N>{{data[Indices]...}};
    }

public:
    std::size_t data[N];
    MyArray<N> copy() const
    {
        // make_index_sequence is introduced in C++14
        return copy_internal(std::make_index_sequence<N>{});
    }
};

class CppTest:public CxxTest::TestSuite
{
    public:
        void setUp() {}
        void tearDown() {}

        void testCpp14() {
            // test std::make_index_sequence
            MyArray<3> a1 {1, 2, 3}, a2 {0, 0, 0};
            a2 = a1.copy();
            TS_ASSERT_EQUALS(a2.data[0], 1);
            TS_ASSERT_EQUALS(a2.data[1], 2);
            TS_ASSERT_EQUALS(a2.data[2], 3);
        }
};

#endif // MSGPARSETEST_H
