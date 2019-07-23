/*
  ZynAddSubFX - a software synthesizer

  AdNoteTest.h - CxxTest for Synth/SUBnote
  Copyright (C) 2009-2011 Mark McCurry
  Author: Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

//Based Upon AdNoteTest.h
#include <cxxtest/TestSuite.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include "../Misc/Master.h"
#include "../Misc/Allocator.h"
#include "../Misc/Util.h"
#include "../Misc/XMLwrapper.h"
#include "../Synth/SUBnote.h"
#include "../Params/SUBnoteParameters.h"
#include "../Params/Presets.h"
#include "../globals.h"
#include <rtosc/thread-link.h>
#include <rtosc/rtosc.h>

using namespace std;
using namespace zyn;

SYNTH_T *synth;

class TriggerTest:public CxxTest::TestSuite
{
    public:

        SUBnoteParameters *pars;
        SUBnote      *note;
        Master       *master;
        AbsTime      *time;
        Controller   *controller;
        unsigned char testnote;
        Alloc         memory;
        rtosc::ThreadLink *tr;
        WatchManager *w;


        float *outR, *outL;

        void setUp() {
            synth = new SYNTH_T;
            // //First the sensible settings and variables that have to be set:
            synth->buffersize = 32;
            synth->alias(false);
            outL = new float[synth->buffersize];
            outR = new float[synth->buffersize];
            for(int i = 0; i < synth->buffersize; ++i) {
                outL[i] = 0;
                outR[i] = 0;
            }

            time  = new AbsTime(*synth);

            tr  = new rtosc::ThreadLink(1024,3);
            w   = new WatchManager(tr);

            //prepare the default settings
            SUBnoteParameters *defaultPreset = new SUBnoteParameters(time);
            sprng(0x7eefdead);

            controller = new Controller(*synth, time);

            //lets go with.... 50! as a nice note
            testnote = 50;
            float freq = 440.0f * powf(2.0f, (testnote - 69.0f) / 12.0f);

            SynthParams pars{memory, *controller, *synth, *time, freq, 120, 0, testnote / 12.0f, false, prng()};
            note = new SUBnote(defaultPreset, pars, w);
            this->pars = defaultPreset;
        }

        void tearDown() {
            delete controller;
            delete note;
            delete [] outL;
            delete [] outR;
            delete time;
            delete synth;
            delete pars;
        }

        void dump_samples(const char *s) {
            puts(s);
            for(int i=0; i<synth->buffersize; ++i)
                printf(w->prebuffer[0][i]>0?"+":"-");
            printf("\n");
            for(int i=0; i<synth->buffersize; ++i)
                printf(w->prebuffer[1][i]>0?"+":"-");
            printf("\n");
            //for(int i=0; i<synth->buffersize; ++i)
            //    printf("%d->%f\n", i, w->prebuffer[0][i]);
            //for(int i=0; i<synth->buffersize; ++i)
            //    printf("%d->%f\n", i, w->prebuffer[1][i]);
        }

        void testCombinedTrigger() {
            //Generate a note
            note->noteout(outL, outR);
            note->releasekey();

            //Preconditions
            //
            //- No pending messages
            //- No active watch points
            //
            TS_ASSERT(!tr->hasNext());
            TS_ASSERT_EQUALS(string(""), w->active_list[0]);
            TS_ASSERT_EQUALS(string(""), w->active_list[1]);
            TS_ASSERT_EQUALS(0, w->sample_list[0]);
            TS_ASSERT_EQUALS(0, w->sample_list[1]);
            TS_ASSERT(!w->trigger_active("noteout"));
            TS_ASSERT(!w->trigger_active("noteout1"));

            //Setup a watchpoint
            //
            // - Watchpoints will be added to the active list in the watch
            // manager
            // - Watchpoints will not be triggered
            w->add_watch("noteout");
            w->add_watch("noteout1");
            TS_ASSERT(!w->trigger_active("noteout"));
            TS_ASSERT(!w->trigger_active("noteout1"));
            TS_ASSERT_EQUALS(string("noteout"),  w->active_list[0]);
            TS_ASSERT_EQUALS(string("noteout1"), w->active_list[1]);
            TS_ASSERT_EQUALS(0, w->sample_list[0]);
            TS_ASSERT_EQUALS(0, w->sample_list[1]);
            dump_samples("Initial pre-buffer");

            //Run the system
            //noteout1 should trigger on this buffer
            note->noteout(outL, outR);
            w->tick();
            dump_samples("Step 1 pre-buffer");
            TS_ASSERT(w->trigger_active("noteout"));
            TS_ASSERT(w->trigger_active("noteout1"));
            TS_ASSERT(!tr->hasNext());
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[0], 32);//only 32 have been
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[1], 32);//processed so far


            //Both should continue to accumulate samples
            note->noteout(outL, outR);
            w->tick();
            dump_samples("Step 2 pre-buffer\n");
            TS_ASSERT(w->trigger_active("noteout1"));
            TS_ASSERT(w->trigger_active("noteout"));
            TS_ASSERT(!tr->hasNext());
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[0], 64);//only 64 have been
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[1], 64);//processed so far

            //Continue accum samples
            note->noteout(outL, outR);
            w->tick();
            dump_samples("Step 3 pre-buffer\n");
            TS_ASSERT(w->trigger_active("noteout1"));
            TS_ASSERT(w->trigger_active("noteout"));
            TS_ASSERT(!tr->hasNext());
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[0], 96);//only 96 have been
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[1], 96);//processed so far

            //Finish accumulating samples
            note->noteout(outL, outR);
            w->tick();
            dump_samples("Step 4 pre-buffer\n");
            TS_ASSERT(!w->trigger_active("noteout1"));
            TS_ASSERT(!w->trigger_active("noteout"));
            TS_ASSERT(tr->hasNext());



#define f32  "ffffffffffffffffffffffffffffffff"
#define f128 f32 f32 f32 f32
            //Verify the output to the user interface
            //if 128 samples are requested, then 128 should be delivered
            const char *msg1 = tr->read();
            TS_ASSERT_EQUALS(string("noteout"), msg1);
            TS_ASSERT_EQUALS(string(f128), rtosc_argument_string(msg1));
            TS_ASSERT_EQUALS(128, strlen(rtosc_argument_string(msg1)));
            TS_ASSERT(tr->hasNext());
            const char *msg2 = tr->read();
            TS_ASSERT_EQUALS(string("noteout1"), msg2);
            TS_ASSERT_EQUALS(128, strlen(rtosc_argument_string(msg2)));
            TS_ASSERT_EQUALS(string(f128), rtosc_argument_string(msg2));
            TS_ASSERT(!tr->hasNext());
        }

};
