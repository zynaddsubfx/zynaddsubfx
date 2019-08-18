/*
  ZynAddSubFX - a software synthesizer
  AdNoteTest.h - CxxTest for Synth/ADnote
  Copyright (C) 2009-2011 Mark McCurry
  Copyright (C) 2009 Harald Hvaal
  Authors: Mark McCurry, Harald Hvaal
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/


#include <cxxtest/TestSuite.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include "../Misc/Master.h"
#include "../Misc/Util.h"
#include "../Misc/Allocator.h"
#include "../Synth/ADnote.h"
#include "../Params/Presets.h"
#include "../DSP/FFTwrapper.h"
#include "../globals.h"
#include <rtosc/thread-link.h>

using namespace std;
using namespace zyn;

SYNTH_T *synth;


class AdNoteTest:public CxxTest::TestSuite
{
    public:
        rtosc::ThreadLink *tr;
        ADnote       *note;
        AbsTime      *time;
        FFTwrapper   *fft;
        ADnoteParameters *defaultPreset;
        Controller   *controller;
        Alloc         memory;
        unsigned char testnote;
        WatchManager *w;
        float *outR, *outL;

        void setUp() {
            //First the sensible settings and variables that have to be set:
            synth = new SYNTH_T;
            synth->buffersize = 256;
            //synth->alias();
            time  = new AbsTime(*synth);

            outL = new float[synth->buffersize];
            for(int i = 0; i < synth->buffersize; ++i)
                *(outL + i) = 0;
            outR = new float[synth->buffersize];
            for(int i = 0; i < synth->buffersize; ++i)
                *(outR + i) = 0;

            tr  = new rtosc::ThreadLink(1024,3);
            w   = new WatchManager(tr);

            fft = new FFTwrapper(synth->oscilsize);
            //prepare the default settings
            defaultPreset = new ADnoteParameters(*synth, fft, time);

            //Assert defaults
            TS_ASSERT(!defaultPreset->VoicePar[1].Enabled);

            XMLwrapper wrap;
            cout << string(SOURCE_DIR) + string("/guitar-adnote.xmz")
                 << endl;
            wrap.loadXMLfile(string(SOURCE_DIR)
                              + string("/guitar-adnote.xmz"));
            TS_ASSERT(wrap.enterbranch("MASTER"));
            TS_ASSERT(wrap.enterbranch("PART", 0));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT"));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT_KIT"));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT_KIT_ITEM", 0));
            TS_ASSERT(wrap.enterbranch("ADD_SYNTH_PARAMETERS"));
            defaultPreset->getfromXML(wrap);
            //defaultPreset->defaults();

            //verify xml was loaded
            TS_ASSERT(defaultPreset->VoicePar[1].Enabled);



            controller = new Controller(*synth, time);

            //lets go with.... 50! as a nice note
            testnote = 50;
            float freq = 440.0f * powf(2.0f, (testnote - 69.0f) / 12.0f);
            SynthParams pars{memory, *controller, *synth, *time, freq, 120, 0, testnote / 12.0f, false, prng()};

            note = new ADnote(defaultPreset, pars,w);

        }

        void tearDown() {
            delete note;
            delete controller;
            delete defaultPreset;
            delete fft;
            delete [] outL;
            delete [] outR;
            FFT_cleanup();
            delete synth;
        }

        void testDefaults() {
            int sampleCount = 0;

//#define WRITE_OUTPUT

#ifdef WRITE_OUTPUT
            ofstream file("adnoteout", ios::out);
#endif
            note->noteout(outL, outR);
#ifdef WRITE_OUTPUT
            for(int i = 0; i < synth->buffersize; ++i)
                file << outL[i] << std::endl;

#endif
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], 0.2555f, 0.0001f);
            note->releasekey();

            TS_ASSERT(!tr->hasNext());
            w->add_watch("noteout/be4_mix");
            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], -0.4688f, 0.0001f);
            w->tick();
            TS_ASSERT(tr->hasNext());
            
            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            w->tick();
            TS_ASSERT_DELTA(outL[255], 0.0613f, 0.0001f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], 0.0971f, 0.0001f);
            w->tick();
            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], -0.0901f, 0.0001f);
            w->tick();

            TS_ASSERT(tr->hasNext());
            TS_ASSERT_EQUALS(string("noteout/be4_mix"), tr->read());
            TS_ASSERT(!tr->hasNext());

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;


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

            TS_ASSERT_EQUALS(sampleCount, 9472);
        }

#define OUTPUT_PROFILE
#ifdef OUTPUT_PROFILE
        void testSpeed() {
            const int samps = 15000;

            int t_on = clock(); // timer before calling func
            for(int i = 0; i < samps; ++i)
                note->noteout(outL, outR);
            int t_off = clock(); // timer when func returns

            printf("AdNoteTest: %f seconds for %d Samples to be generated.\n",
                   (static_cast<float>(t_off - t_on)) / CLOCKS_PER_SEC, samps);
        }
#endif
};
