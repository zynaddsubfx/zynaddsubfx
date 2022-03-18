/*
  ZynAddSubFX - a software synthesizer

  PortamentoTest.h - Test For Portamento
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "test-suite.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include "../Misc/Time.h"
#include "../Misc/Allocator.h"
#define private public
#define protected public
#include "../Synth/SynthNote.h"
#include "../Synth/Portamento.h"
#include "../globals.h"

using namespace std;
using namespace zyn;

#define MAX_PORTAMENTO_LOOPS 1000

SYNTH_T *synth;
int dummy=0;

class PortamentoTest
{
    private:
        AbsTime *time;
        SYNTH_T *synth;
        Controller *ctl;
    public:
        PortamentoTest() {}
        void setUp() {
            synth = new SYNTH_T;
            time  = new AbsTime(*synth);
            ctl = new Controller(*synth, time);
        }

        void testPortamentoRange() {
            //Initialize portamento
            ctl->setportamento(127);
            ctl->portamento.time = 127;
            ctl->portamento.automode = 0;
            Portamento portamento(*ctl, *synth, false, log2f(40.0f), log2f(40.0f), log2f(400.0f));
            TS_ASSERT(portamento.active);
            //Bounds Check
            //We put a bound on number of loops executed, or we could be here
            //a very long time if the exit condition is never fulfilled.
            int loopcount = 0;
            while(portamento.active && ++loopcount < MAX_PORTAMENTO_LOOPS) {
                TS_ASSERT((0.0f <= portamento.x)
                          && (portamento.x <= 1.0f));
                TS_ASSERT((log2f(0.1f) <= portamento.freqdelta_log2)
                          && (portamento.freqdelta_log2 <= log2f(1.0f)));
                portamento.update();
            }
            TS_ASSERT(loopcount < MAX_PORTAMENTO_LOOPS);
            TS_ASSERT((0.0f <= portamento.x)
                      && (portamento.x <= 1.0f));
            TS_ASSERT((log2f(0.1f) <= portamento.freqdelta_log2)
                      && (portamento.freqdelta_log2 <= log2f(1.0f)));
        }

        void testPortamentoValue() {
            ctl->setportamento(127);
            ctl->portamento.time = 127;
            ctl->portamento.automode = 0;
            Portamento portamento(*ctl, *synth, false, log2f(40.0f), log2f(40.0f), log2f(400.0f));
            TS_ASSERT(portamento.active);
            int i;
            for(i = 0; i < 10; ++i)
                portamento.update();
            //Assert that the numbers are the same as they were at release
            TS_ASSERT_DELTA(portamento.x, 0.0290249f, 0.000001f);
            TS_ASSERT_DELTA(portamento.freqdelta_log2, -3.2255092, 0.000001f);
        }

        void tearDown() {
            delete ctl;
            delete time;
            delete synth;
        }
};

int main()
{
    PortamentoTest test;
    RUN_TEST(testPortamentoRange);
    RUN_TEST(testPortamentoValue);
    return test_summary();
}
