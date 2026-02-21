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

    private:
        Controller *testCtl;
};

int main()
{
    return test_summary();
}
