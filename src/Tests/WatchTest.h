/*
  ZynAddSubFX - a software synthesizer

  PluginTest.h - CxxTest for realtime watch points
  Copyright (C) 2015-2015 Mark McCurry
  Authors: Mark McCurry

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
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <rtosc/thread-link.h>
#include "../Misc/Time.h"
#include "../Params/LFOParams.h"
#include "../Synth/LFO.h"
#include "../Synth/SynthNote.h"
#include <unistd.h>
using namespace std;
using namespace zyn;

char *instance_name=(char*)"";

class WatchTest:public CxxTest::TestSuite
{
    public:
        rtosc::ThreadLink *tr;
        SYNTH_T      *s;
        AbsTime      *at;
        WatchManager *w;
        LFOParams    *par;
        LFO          *l;
        void setUp() {
            tr  = new rtosc::ThreadLink(1024,3);
            s   = new SYNTH_T;
            at  = new AbsTime(*s);
            w   = new WatchManager(tr);
            par = new LFOParams;
            l   = new LFO(*par, 440.0, *at, w);

        }

        void tearDown() {
            delete at;
            delete s;
            delete tr;
        }

        void testNoWatch(void)
        {
            TS_ASSERT(!tr->hasNext());
            l->lfoout();
            TS_ASSERT(!tr->hasNext());
        }

        void testPhaseWatch(void)
        {
            TS_ASSERT(!tr->hasNext());
            w->add_watch("out");
            l->lfoout();
            w->tick();
            TS_ASSERT(tr->hasNext());
            TS_ASSERT_EQUALS(string("out"), tr->read());
            TS_ASSERT(!tr->hasNext());
        }

};
