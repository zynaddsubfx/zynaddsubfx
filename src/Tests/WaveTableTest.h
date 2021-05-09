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
#include <cxxtest/TestSuite.h>
using namespace zyn;

class WaveTableTest : public CxxTest::TestSuite
{
    public:

        void testRefs() {
            WaveTable* wt = new WaveTable(1024);
            WaveTableRef ref1(wt);
            TS_ASSERT(ref1.debug_refcount() == 1);
            {
                WaveTableRef ref2(ref1);
                TS_ASSERT(ref1.debug_refcount() == 2);
            }
            TS_ASSERT(ref1.debug_refcount() == 1);
        }
};
