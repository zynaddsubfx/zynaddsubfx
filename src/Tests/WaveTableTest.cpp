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
#include "test-suite.h"
using namespace zyn;

class WaveTableTest
{
    public:
		void setUp() {}
        void testNothing() {
            TS_ASSERT(true);
        }
		void tearDown() {}
};

int main()
{
	WaveTableTest test;
	RUN_TEST(testNothing);
	return test_summary();
}

