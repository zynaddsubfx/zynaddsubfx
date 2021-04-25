/*
  ZynAddSubFX - a software synthesizer

  ControllerTest.h - CxxTest for Params/Controller
  Copyright (C) 2009-2011 Mark McCurry
  Author: Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "test-suite.h"
#include <cmath>
#include <iostream>
#include "../Params/Controller.h"
#include "../globals.h"
#include "../Misc/Time.h"
using namespace zyn;

SYNTH_T *synth;

class ControllerTest
{
    public:
        void setUp() {
            synth   = new SYNTH_T;
            AbsTime time(*synth);
            testCtl = new Controller(*synth, &time);
        }

        void tearDown() {
            delete testCtl;
            delete synth;
        }


        void testPortamentoRange() {
            //Initialize portamento
            testCtl->setportamento(127);
            testCtl->portamento.time = 127;
            testCtl->initportamento(log2f(40.0f), log2f(400.0f), false);
            //Bounds Check
            while(testCtl->portamento.used) {
                TS_ASSERT((0.0f <= testCtl->portamento.x)
                          && (testCtl->portamento.x <= 1.0f));
                TS_ASSERT((log2f(0.1f) <= testCtl->portamento.freqdelta_log2)
                          && (testCtl->portamento.freqdelta_log2 <= log2f(1.0f)));
                testCtl->updateportamento();
            }
            TS_ASSERT((0.0f <= testCtl->portamento.x)
                      && (testCtl->portamento.x <= 1.0f));
            TS_ASSERT((log2f(0.1f) <= testCtl->portamento.freqdelta_log2)
                      && (testCtl->portamento.freqdelta_log2 <= log2f(1.0f)));
        }

        void testPortamentoValue() {
            testCtl->setportamento(127);
            testCtl->portamento.time = 127;
            testCtl->initportamento(log2f(40.0f), log2f(400.0f), false);
            int i;
            for(i = 0; i < 10; ++i)
                testCtl->updateportamento();
            //Assert that the numbers are the same as they were at release
            TS_ASSERT_DELTA(testCtl->portamento.x, 0.0290249f, 0.000001f);
            TS_ASSERT_DELTA(testCtl->portamento.freqdelta_log2, -3.2255092, 0.000001f);
        }

    private:
        Controller *testCtl;
};

int main()
{
    ControllerTest test;
    RUN_TEST(testPortamentoRange);
    RUN_TEST(testPortamentoValue);
    return test_summary();
}
