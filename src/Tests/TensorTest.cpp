/*
  ZynAddSubFX - a software synthesizer

  RandTest.h - CxxTest for Pseudo-Random Number Generator
  Copyright (C) 2009-2009 Mark McCurry
  Author: Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "../Params/WaveTable.h"
#include <cstdlib>
#include <cstdio>
#include "test-suite.h"
using namespace zyn;

class TensorTest
{
        // replacements because TS_ASSERT_EQUALS requires a copy ctor
        template <std::size_t N, class T>
        void MY_TS_ASSERT_EQUALS(const Tensor<N, T>& t1, const Tensor<N, T>& t2)
        {
            TS_ASSERT(t1 == t2);
        }
        template <std::size_t N, class T>
        void MY_TS_ASSERT_DIFFERS(const Tensor<N, T>& t1, const Tensor<N, T>& t2)
        {
            TS_ASSERT(t1 != t2);
        }

    public:

		void setUp() {}
		void tearDown() {}

		void testPowerOf2(void)
        {
			TS_ASSERT_EQUAL_INT(AbstractRingbuffer::debugfunc_is_power_of_2(1), 1);
			TS_ASSERT_EQUAL_INT(AbstractRingbuffer::debugfunc_is_power_of_2(2), 1);
			TS_ASSERT_EQUAL_INT(AbstractRingbuffer::debugfunc_is_power_of_2(4), 1);
			TS_ASSERT_EQUAL_INT(AbstractRingbuffer::debugfunc_is_power_of_2(0), 0);
			TS_ASSERT_EQUAL_INT(AbstractRingbuffer::debugfunc_is_power_of_2(3), 0);
        }

        void testRb(void)
        {
            AbstractRingbuffer rb(4);

            // r=w=0
			TS_ASSERT_EQUAL_INT(4, rb.size());
			TS_ASSERT_EQUAL_INT(0, rb.read_space());
			TS_ASSERT_EQUAL_INT(0, rb.write_space_delayed());
			TS_ASSERT_EQUAL_INT(4, rb.write_space());

			rb.inc_write_pos(2); TS_ASSERT_EQUAL_INT(2, rb.write_space_delayed());
            rb.inc_write_pos_delayed(2);
            // r=0, w=2
			TS_ASSERT_EQUAL_INT(2, rb.read_space());
			TS_ASSERT_EQUAL_INT(2, rb.write_space());

            rb.inc_read_pos(); rb.inc_read_pos();
            // r=w=2
			TS_ASSERT_EQUAL_INT(0, rb.read_space());
			TS_ASSERT_EQUAL_INT(4, rb.write_space());

			rb.inc_write_pos(2); TS_ASSERT_EQUAL_INT(2, rb.write_space_delayed());
            rb.inc_write_pos_delayed(2);
            // r=2, w=0
			TS_ASSERT_EQUAL_INT(2, rb.read_space());
			TS_ASSERT_EQUAL_INT(2, rb.write_space());

			rb.inc_write_pos(1); TS_ASSERT_EQUAL_INT(1, rb.write_space_delayed());
            rb.inc_write_pos_delayed(1);
            // r=2, w=1
			TS_ASSERT_EQUAL_INT(3, rb.read_space());
			TS_ASSERT_EQUAL_INT(1, rb.write_space());

            rb.inc_read_pos(); rb.inc_read_pos();
            // r=0, w=1
			TS_ASSERT_EQUAL_INT(1, rb.read_space());
			TS_ASSERT_EQUAL_INT(3, rb.write_space());

			rb.inc_write_pos(2); TS_ASSERT_EQUAL_INT(2, rb.write_space_delayed());
            rb.inc_write_pos_delayed(2);
            // r=0, w=3
			TS_ASSERT_EQUAL_INT(3, rb.read_space());
			TS_ASSERT_EQUAL_INT(1, rb.write_space());

            rb.inc_read_pos(); rb.inc_read_pos();
            // r=2, w=3
			TS_ASSERT_EQUAL_INT(1, rb.read_space());
			TS_ASSERT_EQUAL_INT(3, rb.write_space());

			rb.inc_write_pos(1); TS_ASSERT_EQUAL_INT(1, rb.write_space_delayed());
            rb.inc_write_pos_delayed(1);
            // r=2, w=0
			TS_ASSERT_EQUAL_INT(2, rb.read_space());
			TS_ASSERT_EQUAL_INT(2, rb.write_space());

            rb.inc_read_pos(); rb.inc_read_pos();
            // r=0, w=0
			TS_ASSERT_EQUAL_INT(0, rb.read_space());
			TS_ASSERT_EQUAL_INT(4, rb.write_space());

			rb.inc_write_pos(2); TS_ASSERT_EQUAL_INT(2, rb.write_space_delayed());
            rb.inc_write_pos_delayed(2);
            // r=0, w=2
			TS_ASSERT_EQUAL_INT(2, rb.read_space());
			TS_ASSERT_EQUAL_INT(2, rb.write_space());

			rb.inc_write_pos(1); TS_ASSERT_EQUAL_INT(1, rb.write_space_delayed());
            rb.inc_write_pos_delayed(1);
            // r=0, w=3
			TS_ASSERT_EQUAL_INT(3, rb.read_space());
			TS_ASSERT_EQUAL_INT(1, rb.write_space());

            rb.inc_read_pos();
            // r=1, w=3
			TS_ASSERT_EQUAL_INT(2, rb.read_space());
			TS_ASSERT_EQUAL_INT(2, rb.write_space());

			rb.inc_write_pos(1); TS_ASSERT_EQUAL_INT(1, rb.write_space_delayed());
            rb.inc_write_pos_delayed(1);
            // r=1, w=0
			TS_ASSERT_EQUAL_INT(3, rb.read_space());
			TS_ASSERT_EQUAL_INT(1, rb.write_space());

            rb.inc_read_pos();
            // r=2, w=0
			TS_ASSERT_EQUAL_INT(2, rb.read_space());
			TS_ASSERT_EQUAL_INT(2, rb.write_space());
        }

        void testRb2(void)
        {
            // this is really a strange case
            // normal ringbuffers would not allow writing anything, because
            // when you write, the write pointer would hit the read pointer
            // this ringbuffer simply allows writing one element
            AbstractRingbuffer rb(1);

            // r=w=0
			TS_ASSERT_EQUAL_INT(1, rb.size());

			TS_ASSERT_EQUAL_INT(rb.read_pos(), 0);
			TS_ASSERT_EQUAL_INT(rb.write_pos_delayed(), 0);
			TS_ASSERT_EQUAL_INT(rb.write_pos(), 0);
			TS_ASSERT_EQUAL_INT(0, rb.read_space());
			TS_ASSERT_EQUAL_INT(0, rb.write_space_delayed());
			TS_ASSERT_EQUAL_INT(1, rb.write_space());

			rb.inc_write_pos(1); TS_ASSERT_EQUAL_INT(1, rb.write_space_delayed());
            rb.inc_write_pos_delayed(1);

            // r=0, w=1
			TS_ASSERT_EQUAL_INT(rb.read_pos(), 0);
            // still 0, but now "before r" in cyclic sense
			TS_ASSERT_EQUAL_INT(rb.write_pos_delayed(), 0);
			TS_ASSERT_EQUAL_INT(rb.write_pos(), 0);
			TS_ASSERT_EQUAL_INT(1, rb.read_space());
			TS_ASSERT_EQUAL_INT(0, rb.write_space());

            rb.inc_read_pos();
            // r=w=1
			TS_ASSERT_EQUAL_INT(0, rb.read_space());
			TS_ASSERT_EQUAL_INT(1, rb.write_space());

			rb.inc_write_pos(1); TS_ASSERT_EQUAL_INT(1, rb.write_space_delayed());
            rb.inc_write_pos_delayed(1);

            // r=1, w=0
			TS_ASSERT_EQUAL_INT(rb.read_pos(), 0);
			TS_ASSERT_EQUAL_INT(rb.write_pos_delayed(), 0);
			TS_ASSERT_EQUAL_INT(rb.write_pos(), 0);
			TS_ASSERT_EQUAL_INT(1, rb.read_space());
			TS_ASSERT_EQUAL_INT(0, rb.write_space());

            rb.inc_read_pos();
            // r=w=0
			TS_ASSERT_EQUAL_INT(rb.read_pos(), 0);
			TS_ASSERT_EQUAL_INT(rb.write_pos_delayed(), 0);
			TS_ASSERT_EQUAL_INT(rb.write_pos(), 0);
			TS_ASSERT_EQUAL_INT(0, rb.read_space());
			TS_ASSERT_EQUAL_INT(1, rb.write_space());
        }

        void testShapeEquals(void) {
            Shape3 s1{3,4,5}, s2{3,4,5};;
			TS_ASSERT(s1 == s2);
        }

        void testShapeDiffers(void) {
            Shape3 s1{3,4,5}, s2{3,3,5};;
			TS_ASSERT(s1 != s2);
        }

        void testShapeVolume(void) {
            Shape3 s{3,4,5};
			TS_ASSERT_EQUAL_INT(s.volume(), 60);
        }

        void testShapeProj(void) {
            Shape3 s{3,4,5};
            Shape2 exp1{4,5};
            Shape1 exp2{5};
			TS_ASSERT(s.proj() == exp1);
			TS_ASSERT(s.proj().proj() == exp2);
        }

        void testShapePrepend(void) {
            Shape1 s{5};
            Shape2 exp1{4,5};
            Shape3 exp2{3,4,5};
			TS_ASSERT(s.prepend_dim(4) == exp1);
			TS_ASSERT(s.prepend_dim(4).prepend_dim(3) == exp2);
        }

        void testTensorSetData() {
            Shape3 s; s.dim[0] = s.dim[1] = s.dim[2] = 2;
            int raw[] = {0,1,2,3,4,5,6,7};
            Tensor3<int> t(s);
            std::size_t consumed = t.debug_set_data_using_deep_copy(raw);
			TS_ASSERT_EQUAL_INT(consumed, sizeof (raw) / sizeof (raw[0]));
			TS_ASSERT_EQUAL_INT(t[0][0][0], 0);
			TS_ASSERT_EQUAL_INT(t[0][0][1], 1);
			TS_ASSERT_EQUAL_INT(t[0][1][0], 2);
			TS_ASSERT_EQUAL_INT(t[0][1][1], 3);
			TS_ASSERT_EQUAL_INT(t[1][0][0], 4);
			TS_ASSERT_EQUAL_INT(t[1][0][1], 5);
			TS_ASSERT_EQUAL_INT(t[1][1][0], 6);
			TS_ASSERT_EQUAL_INT(t[1][1][1], 7);
        }

        void testTensorEquals() {
            Shape3 s; s.dim[0] = s.dim[1] = s.dim[2] = 2;
            int raw[] = {0,1,2,3,4,5,6,7};
            Tensor3<int> t1(s);
            Tensor3<int> t2(s);
            t1.debug_set_data_using_deep_copy(raw);
            t2.debug_set_data_using_deep_copy(raw);
            MY_TS_ASSERT_EQUALS(t1, t2);
        }

        void testTensorDiffers() {
            Shape3 s;
            s.dim[0] = s.dim[1] = s.dim[2] = 2;
            int raw1[] = {0,1,2,3,4,5,6,7};
            int raw2[] = {0,1,2,3,4,4,6,7};
            Tensor3<int> t1(s);
            Tensor3<int> t2(s);
            t1.debug_set_data_using_deep_copy(raw1);
            t2.debug_set_data_using_deep_copy(raw2);
            MY_TS_ASSERT_DIFFERS(t1, t2);
        }

        void testTensorSlice() {
            Shape3 s{2,2,2};
            int raw[] = {0,1,2,3,4,5,6,7};
            Tensor3<int> t3(s);
            t3.debug_set_data_using_deep_copy(raw);
            const Tensor2<int>& t20 = t3[0];
            const Tensor2<int>& t21 = t3[1];
            const Tensor1<int>& t10 = t20[0];
            const Tensor1<int>& t11 = t20[1];
            int exp20[] = {0,1,2,3};
            int exp21[] = {4,5,6,7};
            int exp10[] = {0,1};
            int exp11[] = {2,3};
            Tensor2<int> texp20(t20.debug_get_shape()); texp20.debug_set_data_using_deep_copy(exp20);
            Tensor2<int> texp21(t20.debug_get_shape()); texp21.debug_set_data_using_deep_copy(exp21);
            Tensor1<int> texp10(t10.debug_get_shape()); texp10.debug_set_data_using_deep_copy(exp10);
            Tensor1<int> texp11(t10.debug_get_shape()); texp11.debug_set_data_using_deep_copy(exp11);
            MY_TS_ASSERT_EQUALS(t20, texp20);
            MY_TS_ASSERT_EQUALS(t21, texp21);
            MY_TS_ASSERT_EQUALS(t10, texp10);
            MY_TS_ASSERT_EQUALS(t11, texp11);
			TS_ASSERT_EQUAL_INT(t11[0], 2);
			TS_ASSERT_EQUAL_INT(t11[1], 3);
        }

        void testFindBestIndex(){
            wavetable_types::float32 raw[] = { 55.f, 110.f, 220.f };
            Tensor<1, wavetable_types::float32> freqs(sizeof(raw)/sizeof(raw[0]));
            freqs.debug_set_data_using_deep_copy(raw);
			TS_ASSERT_EQUAL_INT(findBestIndex(freqs, -1.f), 0);
			TS_ASSERT_EQUAL_INT(findBestIndex(freqs, 0.f), 0);
			TS_ASSERT_EQUAL_INT(findBestIndex(freqs, 110.f), 1);
			TS_ASSERT_EQUAL_INT(findBestIndex(freqs, 220.f), 2);
			TS_ASSERT_EQUAL_INT(findBestIndex(freqs, 440.f), 2);
        }
};

int main()
{
	TensorTest test;
	RUN_TEST(testPowerOf2);
	RUN_TEST(testRb);
	RUN_TEST(testRb2);
	RUN_TEST(testShapeEquals);
	RUN_TEST(testShapeDiffers);
	RUN_TEST(testShapeVolume);
	RUN_TEST(testShapeProj);
	RUN_TEST(testShapePrepend);
	RUN_TEST(testTensorSetData);
	RUN_TEST(testTensorEquals);
	RUN_TEST(testTensorDiffers);
	RUN_TEST(testTensorSlice);
	RUN_TEST(testFindBestIndex);
	return test_summary();
}


