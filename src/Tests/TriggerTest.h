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
            for(int i = 0; i < synth->buffersize; ++i)
                *(outL + i) = 0;
            outR = new float[synth->buffersize];
            for(int i = 0; i < synth->buffersize; ++i)
                *(outR + i) = 0;


            time  = new AbsTime(*synth);

            tr  = new rtosc::ThreadLink(1024,3);
            w   = new WatchManager(tr);

            //prepare the default settings
            SUBnoteParameters *defaultPreset = new SUBnoteParameters(time);
            XMLwrapper wrap;
            wrap.loadXMLfile(string(SOURCE_DIR)
                              + string("/guitar-adnote.xmz"));
            TS_ASSERT(wrap.enterbranch("MASTER"));
            TS_ASSERT(wrap.enterbranch("PART", 1));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT"));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT_KIT"));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT_KIT_ITEM", 0));
            TS_ASSERT(wrap.enterbranch("SUB_SYNTH_PARAMETERS"));
            defaultPreset->getfromXML(wrap);
            
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

        void testDefaults() {
            //Note: if these tests fail it is due to the relationship between
            //global.h::RND and SUBnote.cpp

            int sampleCount = 0;

//#define WRITE_OUTPUT

#ifdef WRITE_OUTPUT
            ofstream file("subnoteout", ios::out);
#endif
            note->noteout(outL, outR);
#ifdef WRITE_OUTPUT
            for(int i = 0; i < synth->buffersize; ++i)
                file << outL[i] << std::endl;

#endif
            sampleCount += synth->buffersize;
            note->releasekey();
            TS_ASSERT(!tr->hasNext());
            w->add_watch("noteout");
            w->add_watch("noteout1");
            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            //w->tick();
            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            //w->tick();
            TS_ASSERT(w->trigger_active("noteout1"));
            TS_ASSERT(w->trigger_active("noteout"));
            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            w->tick();
            TS_ASSERT_EQUALS(string("noteout1"), tr->read());
            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            w->tick();
            
            while(!note->finished()) {
                note->noteout(outL, outR);
#ifdef WRITE_OUTPUT
                for(int i = 0; i < synth->buffersize; ++i)
                    file << outL[i] << std::endl;

#endif
                sampleCount += synth->buffersize;
            }
#ifdef WRITE_OUTPUT
            file.close();
#endif
        }

#define OUTPUT_PROFILE
#ifdef OUTPUT_PROFILE
        void testSpeed() {
            const int samps = 15000;

            int t_on = clock(); // timer before calling func
            for(int i = 0; i < samps; ++i)
                note->noteout(outL, outR);
            int t_off = clock(); // timer when func returns

            printf("SubNoteTest: %f seconds for %d Samples to be generated.\n",
                   (static_cast<float>(t_off - t_on)) / CLOCKS_PER_SEC, samps);
        }
#endif
};
