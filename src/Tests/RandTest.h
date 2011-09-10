/*
  ZynAddSubFX - a software synthesizer

  RandTest.h - CxxTest for Pseudo-Random Number Generator
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

#include "../globals.h"

#include <cstdlib>
#include <cstdio>
#include <cxxtest/TestSuite.h>

class RandTest:public CxxTest::TestSuite
{
    public:
        void testPRNG(void) {
            //verify RND returns expected pattern when unseeded
            TS_ASSERT_DELTA(RND, 0.840188, 0.00001);
            TS_ASSERT_DELTA(RND, 0.394383, 0.00001);
            TS_ASSERT_DELTA(RND, 0.783099, 0.00001);
            TS_ASSERT_DELTA(RND, 0.798440, 0.00001);
            TS_ASSERT_DELTA(RND, 0.911647, 0.00001);
        }
};


