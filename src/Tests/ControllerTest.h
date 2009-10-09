/*
  ZynAddSubFX - a software synthesizer

  ControllerTest.h - CxxTest for Params/Controller
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
#include <iostream>
#include "../Params/Controller.h"

class ControllerTest:public CxxTest::TestSuite
{
    public:
        void setUp() {
            testCtl = new Controller();
        }

        void tearDown() {
            delete testCtl;
        }


        void testPortamentoRange() {
            //Initialize portamento
            testCtl->setportamento(127);
            testCtl->portamento.time = 127;
            testCtl->initportamento(40.0, 400.0, false);
            //Bounds Check
            while(testCtl->portamento.used) {
                TS_ASSERT((0.0 <= testCtl->portamento.x)
                          && (testCtl->portamento.x <= 1.0));
                TS_ASSERT((0.1 <= testCtl->portamento.freqrap)
                          && (testCtl->portamento.freqrap <= 1.0));
                testCtl->updateportamento();
            }
            TS_ASSERT((0.0 <= testCtl->portamento.x)
                      && (testCtl->portamento.x <= 1.0));
            TS_ASSERT((0.1 <= testCtl->portamento.freqrap)
                      && (testCtl->portamento.freqrap <= 1.0));
        }

        void testPortamentoValue() {
            testCtl->setportamento(127);
            testCtl->portamento.time = 127;
            testCtl->initportamento(40.0, 400.0, false);
            int i;
            for(i = 0; i < 10; ++i)
                testCtl->updateportamento();
            //Assert that the numbers are the same as they were at release
            TS_ASSERT_DELTA(testCtl->portamento.x, 0.0290249, 0.000001)
            TS_ASSERT_DELTA(testCtl->portamento.freqrap, 0.126122, 0.000001)
        }

    private:
        Controller *testCtl;
};

