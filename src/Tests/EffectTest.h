/*
  ZynAddSubFX - a software synthesizer

  EffectTest.h - CxxTest for General Effect Stuff
  Copyright (C) 2015 Mark McCurry

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

*/
#include <cxxtest/TestSuite.h>
#include <cmath>
#include <cstdio>
#include "../Misc/Allocator.h"
#include "../Misc/Stereo.h"
#include "../Effects/EffectMgr.h"
#include "../Effects/Reverb.h"
#include "../Effects/Echo.h"
#include "../globals.h"
SYNTH_T *synth;

class EchoTest:public CxxTest::TestSuite
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
            TS_ASSERT_EQUALS(mgr->nefx, 0);
            mgr->changeeffect(1);
            TS_ASSERT_EQUALS(mgr->nefx, 1);
            TS_ASSERT_EQUALS(mgr->efx, nullptr);
            mgr->init();
            TS_ASSERT_DIFFERS(mgr->efx, nullptr);
        }

        void testClear() {
            mgr->changeeffect(1);
            mgr->init();
            TS_ASSERT_DIFFERS(mgr->efx, nullptr);
            mgr->changeeffect(0);
            mgr->init();
            TS_ASSERT_EQUALS(mgr->efx, nullptr);
        }

        void testSwap() {
            //Initially the effect is NULL
            TS_ASSERT_EQUALS(mgr->efx, nullptr);

            //A Reverb is selected
            mgr->changeeffect(1);
            mgr->init();
            TS_ASSERT_DIFFERS(dynamic_cast<Reverb*>(mgr->efx), nullptr);
            TS_ASSERT_EQUALS(dynamic_cast<Echo*>(mgr->efx), nullptr);

            //An Echo is selected
            mgr->changeeffect(2);
            mgr->init();
            TS_ASSERT_EQUALS(dynamic_cast<Reverb*>(mgr->efx), nullptr);
            TS_ASSERT_DIFFERS(dynamic_cast<Echo*>(mgr->efx), nullptr);
        }

    private:
        EffectMgr *mgr;
        Allocator *alloc;
        SYNTH_T   *synth;
};
