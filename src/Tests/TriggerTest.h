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
        float test_freq_log2;
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
            sprng(3543);

            controller = new Controller(*synth, time);

            //lets go with.... 50! as a nice note
            test_freq_log2 = log2f(440.0f) + (50.0 - 69.0f) / 12.0f;

            SynthParams pars{memory, *controller, *synth, *time, 120, 0, test_freq_log2, false, prng()};
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

        void testSine(void) {
            //Generate a sine table
            float data[1024] = {};
            for(int i=0; i<1024; ++i)
                data[i] = -sin(2*M_PI*(i/1024.0));

            //Preconditions
            //
            //- No pending messages
            //- No active watch points
            //
            TS_ASSERT(!tr->hasNext());
            TS_ASSERT_EQUALS(string(""), w->active_list[0]);
            TS_ASSERT_EQUALS(0, w->sample_list[0]);
            TS_ASSERT(!w->trigger_active("data"));


            w->add_watch("noteout/filter");
            for(int i=0; i<1024; ++i) {
                w->satisfy("noteout/filter", &data[i], 1);
                w->tick();
            }
            const char *msg1 = tr->read();
            float buf1[128] = {};
            TS_ASSERT(msg1);
            TS_ASSERT_EQUALS(127, rtosc_narguments(msg1));

            printf("msg1 = %s\n",   msg1);
            printf("msg1 = <%s>\n", rtosc_argument_string(msg1));
            printf("nargs = %d\n",  rtosc_narguments(msg1));
            for(int i=0; i<127; ++i)
                buf1[i] = rtosc_argument(msg1, i).f;

            w->add_watch("noteout/amp_int");
            for(int i=0; i<1024/97; ++i) {
                w->satisfy("noteout/amp_int", &data[i*97], 97);
                w->tick();
            }
            const char *msg2 = tr->read();
            TS_ASSERT(msg2);
            TS_ASSERT_EQUALS(127, rtosc_narguments(msg2));
            float buf2[128] = {};
            printf("nargs = %d\n", rtosc_narguments(msg2));
            for(int i=0; i<127; ++i)
                buf2[i] = rtosc_argument(msg2, i).f;
            for(int i=0; i<127; ++i){
               TS_ASSERT_EQUALS(buf1[i], buf2[i]);
                TS_ASSERT_EQUALS(buf1[i],data[450+i]);
                TS_ASSERT_EQUALS(buf2[i],data[450+i]);
            }
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
            TS_ASSERT(!w->trigger_active("noteout/filter"));
            TS_ASSERT(!w->trigger_active("noteout/amp_int"));

            //Setup a watchpoint
            //
            // - Watchpoints will be added to the active list in the watch
            // manager
            // - Watchpoints will not be triggered
            w->add_watch("noteout/filter");
            w->add_watch("noteout/amp_int");
            TS_ASSERT(!w->trigger_active("noteout/filter"));
            TS_ASSERT(!w->trigger_active("noteout/amp_int"));
            TS_ASSERT_EQUALS(string("noteout/filter"),  w->active_list[0]);
            TS_ASSERT_EQUALS(string("noteout/amp_int"), w->active_list[1]);
            TS_ASSERT_EQUALS(0, w->sample_list[0]);
            TS_ASSERT_EQUALS(0, w->sample_list[1]);
            dump_samples("Initial pre-buffer");

            //Run the system
            //noteout1 should trigger on this buffer
            note->noteout(outL, outR);

            w->tick();
            dump_samples("Step 1 pre-buffer");
            TS_ASSERT(!w->trigger_active("noteout/filter"));   //not active as prebuffer is not filled
            TS_ASSERT(!w->trigger_active("noteout/amp_int"));
            TS_ASSERT(!tr->hasNext());
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[0], 0); // Is 0 as prebuffer not filled
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[1], 0);


            //Both should continue to accumulate samples
            note->noteout(outL, outR);
            w->tick();
            dump_samples("Step 2 pre-buffer\n");
            TS_ASSERT(!w->trigger_active("noteout/filter"));   //not active as prebuffer is not filled
            TS_ASSERT(!w->trigger_active("noteout/amp_int"));
            TS_ASSERT(!tr->hasNext());
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[0], 0); // Is 0 as prebuffer not filled
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[1], 0);

            //Continue accum samples
            note->noteout(outL, outR);
            w->tick();
            dump_samples("Step 3 pre-buffer\n");
            TS_ASSERT(!w->trigger_active("noteout/filter"));
            TS_ASSERT(!w->trigger_active("noteout/amp_int"));
            TS_ASSERT(!tr->hasNext());
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[0], 0); // Is 0 as prebuffer not filled
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[1], 0);

            //Finish accumulating samples
            note->noteout(outL, outR);
            w->tick();
            dump_samples("Step 4 pre-buffer\n");
            TS_ASSERT(w->trigger_active("noteout/filter"));   // trigger activate and filling post buffer
            TS_ASSERT(w->trigger_active("noteout/amp_int"));
            TS_ASSERT(!tr->hasNext());  // post buffer not reach 128
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[1], 128);   // prebuffer + postbuffer filled in
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[0], 128);
            note->noteout(outL, outR);
            w->tick();
            note->noteout(outL, outR);
            w->tick();

#define f32  "ffffffffffffffffffffffffffffffff"
#define f128 f32 f32 f32 f32
            //Verify the output to the user interface
            //if 128 samples are requested, then 128 should be delivered
            const char *msg1 = tr->read();
            TS_ASSERT_EQUALS(string("noteout/filter"), msg1);
            TS_ASSERT_EQUALS(string(f128), rtosc_argument_string(msg1));
            TS_ASSERT_EQUALS(128, strlen(rtosc_argument_string(msg1)));
            note->noteout(outL, outR);
            w->tick();
            TS_ASSERT(tr->hasNext());
            const char *msg2 = tr->read();
            TS_ASSERT_EQUALS(string("noteout/amp_int"), msg2);
            TS_ASSERT_EQUALS(128, strlen(rtosc_argument_string(msg2)));
            TS_ASSERT_EQUALS(string(f128), rtosc_argument_string(msg2));
            TS_ASSERT(!tr->hasNext());
        }

};
