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
            sprng(3543);

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

        void testSine(void) {
            //Generate a sine table
            float data[1024] = {0};
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


            w->add_watch("noteout");
            for(int i=0; i<1024; ++i) {
                w->satisfy("noteout", &data[i], 1);
                w->tick();
            }
            const char *msg1 = tr->read();
            float buf1[128] = {0};
            TS_ASSERT(msg1);
            TS_ASSERT_EQUALS(128, rtosc_narguments(msg1));

            printf("msg1 = %s\n",   msg1);
            printf("msg1 = <%s>\n", rtosc_argument_string(msg1));
            printf("nargs = %d\n",  rtosc_narguments(msg1));
            for(int i=0; i<126; ++i)
                buf1[i] = rtosc_argument(msg1, i).f;

            w->add_watch("noteout2");
            for(int i=0; i<1024/32; ++i) {
                w->satisfy("noteout2", &data[i*32], 32);
                w->tick();
            }
            const char *msg2 = tr->read();
            TS_ASSERT(msg2);
            TS_ASSERT_EQUALS(128, rtosc_narguments(msg2));
            float buf2[128] = {0};
            printf("nargs = %d\n", rtosc_narguments(msg2));
            for(int i=0; i<126; ++i)
                buf2[i] = rtosc_argument(msg2, i).f;
            for(int i=0; i<127; ++i){
                //if(buf1[i] != buf2[i]){
                printf("index: %d   %f %f\n", i, buf1[i], buf2[i]);
               // printf("error = %f for %f %f\n", fabsf(buf1[i]-buf2[i]), buf1[i], buf2[i]); 
               // }
            }

        // for (int k = 0; k < 1024; k++){
        //     if(data[k] == buf1[0])
        //         printf("\n index of data for buf1 0 is %d \n",k);
        //     if(data[k] == buf2[0])
        //     printf("\n index of data for buf2 0 is %d \n",k);
        //     }
            //printf("\n ms1 %s  , ms2 %s \n",msg1,msg2);
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
            TS_ASSERT(!w->trigger_active("noteout"));   //not active as prebuffer is not filled
            TS_ASSERT(!w->trigger_active("noteout1"));
            TS_ASSERT(!tr->hasNext());
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[0], 0); // Is 0 as prebuffer not filled
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[1], 0);


            //Both should continue to accumulate samples
            note->noteout(outL, outR);
            w->tick();
            dump_samples("Step 2 pre-buffer\n");
            TS_ASSERT(!w->trigger_active("noteout1")); // not active as prebuffer is not filled
            TS_ASSERT(!w->trigger_active("noteout"));
            TS_ASSERT(!tr->hasNext());
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[0], 0); // Is 0 as prebuffer not filled
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[1], 0);

            //Continue accum samples
            note->noteout(outL, outR);
            w->tick();
            dump_samples("Step 3 pre-buffer\n");
            TS_ASSERT(!w->trigger_active("noteout1"));
            TS_ASSERT(!w->trigger_active("noteout"));
            TS_ASSERT(!tr->hasNext());
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[0], 0); // Is 0 as prebuffer not filled
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[1], 0);
            
            //Finish accumulating samples
            note->noteout(outL, outR);
            w->tick();
            dump_samples("Step 4 pre-buffer\n");
            TS_ASSERT(w->trigger_active("noteout1")); // trigger activate and filling post buffer
            TS_ASSERT(w->trigger_active("noteout"));
            TS_ASSERT(!tr->hasNext());  // post buffer not reach 128
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[1], 128);   // prebuffer + postbuffer filled in
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[0], 128);
            
            note->noteout(outL, outR);
            w->tick();
            dump_samples("Step 5 post-buffer filled\n");
            TS_ASSERT(!w->trigger_active("noteout1")); // trigger deactivate as post-buffer filled
            TS_ASSERT(!w->trigger_active("noteout"));
            TS_ASSERT(tr->hasNext());  // data is sent
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[1], 0);   // overpass 128 and reset to 0
            TS_ASSERT_LESS_THAN_EQUALS(w->sample_list[0], 0);



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
