/*
  ZynAddSubFX - a software synthesizer

  EffectTest.h - CxxTest for General Effect Stuff
  Copyright (C) 2015 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "test-suite.h"
#include <cmath>
#include <cstdio>
#include "../Misc/Allocator.h"
#include "../Misc/Stereo.h"
#include "../Effects/EffectMgr.h"
#include "../Effects/Reverb.h"
#include "../Effects/Echo.h"
#include "../globals.h"
using namespace zyn;

SYNTH_T *synth;

class EffectTest
{
    public:
        void setUp() {
            synth = new SYNTH_T;
            alloc = new AllocatorClass;
            mgr   = new EffectMgr(*alloc, *synth, true);
        }

        void tearDown() {
            delete mgr;
            delete alloc;
            delete synth;
        }

        void testInit() {
            TS_ASSERT_EQUAL_INT(mgr->nefx, 0);
            mgr->changeeffect(1);
            TS_ASSERT_EQUAL_INT(mgr->nefx, 1);
            assert_ptr_eq(mgr->efx, nullptr,
                    "nothing before init", __LINE__);
            mgr->init();
            TS_NON_NULL(mgr->efx);
        }

        void testClear() {
            mgr->changeeffect(1);
            mgr->init();
            TS_NON_NULL(mgr->efx);
            mgr->changeeffect(0);
            mgr->init();
            assert_ptr_eq(mgr->efx, nullptr,
                    "nothing after clearing", __LINE__);
        }

        void testSwap() {
            //Initially the effect is NULL
            assert_ptr_eq(mgr->efx, nullptr,
                    "initially null", __LINE__);

            //A Reverb is selected
            mgr->changeeffect(1);
            mgr->init();
            TS_NON_NULL(dynamic_cast<Reverb*>(mgr->efx));
            assert_ptr_eq(dynamic_cast<Echo*>(mgr->efx),
                    nullptr,
                    "not an echo", __LINE__);

            //An Echo is selected
            mgr->changeeffect(2);
            mgr->init();
            assert_ptr_eq(dynamic_cast<Reverb*>(mgr->efx),
                    nullptr,
                    "not a reverb", __LINE__);
            TS_NON_NULL(dynamic_cast<Echo*>(mgr->efx));
        }

    private:
        EffectMgr *mgr;
        Allocator *alloc;
        SYNTH_T   *synth;
};

int main()
{
    EffectTest test;
    RUN_TEST(testInit);
    RUN_TEST(testClear);
    RUN_TEST(testSwap);
    return test_summary();
}
