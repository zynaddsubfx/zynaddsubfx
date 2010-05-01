/*
  ZynAddSubFX - a software synthesizer

  SampleTest.h - CxxTest for Samples
  Copyright (C) 2009-2009 Mark McCurry
  Author: Mark McCurry

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/
#include <cxxtest/TestSuite.h>
#include "../Samples/Sample.h"

class SampleTest:public CxxTest::TestSuite
{
    public:
        void testInit() {
            Sample smp(10);
            TS_ASSERT_EQUALS(smp.size(), 10);
            for(int i = 0; i < 20; ++i)
                TS_ASSERT_EQUALS(smp[i], 0.0);
            Sample nsmp(5, 15.0);
            TS_ASSERT_EQUALS(nsmp.size(), 5);
            TS_ASSERT_EQUALS(nsmp[4], 15.0);
        }

        void testAssign() {
            Sample smp(3);
            smp[0] = 0;
            smp[1] = 1;
            smp[2] = 2;
            Sample nsmp(40);
            nsmp   = smp;
            TS_ASSERT_EQUALS(smp.size(), nsmp.size());
            for(int i = 0; i < 29; ++i)
                TS_ASSERT_EQUALS(smp[i], nsmp[i]);
        }
        void testBounds() {
            Sample smp(0);
            TS_ASSERT(smp.size() != 0);
        }

        void testAllocDealloc() {
            float *fl = new float[50];
            for(int i = 0; i < 50; ++i)
                *(fl + i) = i;
            Sample smp(2);
            smp = Sample(50, fl);
            delete [] fl;
            for(int i = 0; i < 50; ++i)
                TS_ASSERT_DELTA(smp[i], i, 0.001);
            smp = Sample(3);
        }

        void testClear() {
            Sample smp(50);
            for(int i = 0; i < 50; ++i)
                smp[i] = 10;
            smp.clear();
            for(int i = 0; i < 50; ++i)
                TS_ASSERT_EQUALS(smp[i], 0);
        }

        void testAppend() {
            Sample smp1(54, 2);
            Sample smp2(20, 17);
            smp1.append(smp2);
            TS_ASSERT_EQUALS(smp1.size(), 74);
            for(int i = 0; i < 74; ++i)
                TS_ASSERT_DELTA(smp1[i], (i < 54 ? 2 : 17), 0.001);
        }
};

