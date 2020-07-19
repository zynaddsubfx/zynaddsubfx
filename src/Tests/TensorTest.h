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
#include <cxxtest/TestSuite.h>
using namespace zyn;

class TensorTest : public CxxTest::TestSuite
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
        void testShapeEquals(void) {
            Shape3 s1{3,4,5}, s2{3,4,5};;
            TS_ASSERT_EQUALS(s1, s2);
        }

        void testShapeDiffers(void) {
            Shape3 s1{3,4,5}, s2{3,3,5};;
            TS_ASSERT_DIFFERS(s1, s2);
        }

        void testShapeVolume(void) {
            Shape3 s{3,4,5};
            TS_ASSERT_EQUALS(s.volume(), 60);
        }

        void testShapeProj(void) {
            Shape3 s{3,4,5};
            Shape2 exp1{4,5};
            Shape1 exp2{5};
            TS_ASSERT_EQUALS(s.proj(), exp1);
            TS_ASSERT_EQUALS(s.proj().proj(), exp2);
        }

        void testTensorEquals() {
            Shape3 s; s.dim[0] = s.dim[1] = s.dim[2] = 2;
            int raw[] = {0,1,2,3,4,5,6,7};
            Tensor3<int> t1(s, raw);
            Tensor3<int> t2(s, raw);
            MY_TS_ASSERT_EQUALS(t1, t2);
        }

        void testTensorDiffers() {
            Shape3 s;
            s.dim[0] = s.dim[1] = s.dim[2] = 2;
            int raw1[] = {0,1,2,3,4,5,6,7};
            int raw2[] = {0,1,2,3,4,4,6,7};
            Tensor3<int> t1(s, raw1);
            Tensor3<int> t2(s, raw2);
            MY_TS_ASSERT_DIFFERS(t1, t2);
        }

        void testTensorSlice() {
            Shape3 s{2,2,2};
            int raw[] = {0,1,2,3,4,5,6,7};
            Tensor3<int> t3(s, raw);
            Tensor2<int> t20 = t3[0];
            Tensor2<int> t21 = t3[1];
            Tensor1<int> t10 = t20[0];
            Tensor1<int> t11 = t20[1];
            int exp20[] = {0,1,2,3};
            int exp21[] = {4,5,6,7};
            int exp10[] = {0,1};
            int exp11[] = {2,3};
            MY_TS_ASSERT_EQUALS(t20, Tensor2<int>(t20.shape(), exp20));
            MY_TS_ASSERT_EQUALS(t21, Tensor2<int>(t20.shape(), exp21));
            MY_TS_ASSERT_EQUALS(t10, Tensor1<int>(t10.shape(), exp10));
            MY_TS_ASSERT_EQUALS(t11, Tensor1<int>(t10.shape(), exp11));
            TS_ASSERT_EQUALS(t11[0], 2);
            TS_ASSERT_EQUALS(t11[1], 3);
        }
};
