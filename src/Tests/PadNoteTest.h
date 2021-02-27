/*
  ZynAddSubFX - a software synthesizer

  PadNoteTest.h - CxxTest for Synth/PADnote
  Copyright (C) 20012 zco
  Author: zco

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/


//Based Upon AdNoteTest.h and SubNoteTest.h
#include <cxxtest/TestSuite.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#define private public
#include "../Misc/Master.h"
#include "../Misc/Util.h"
#include "../Misc/Allocator.h"
#include "../Misc/XMLwrapper.h"
#include "../Synth/PADnote.h"
#include "../Synth/OscilGen.h"
#include "../Params/PADnoteParameters.h"
#include "../Params/Presets.h"
#include "../DSP/FFTwrapper.h"
#include "../globals.h"
#include <rtosc/thread-link.h>
using namespace std;
using namespace zyn;

SYNTH_T *synth;

#ifndef SOURCE_DIR
#define SOURCE_DIR "BE QUIET COMPILER"
#endif

class PadNoteTest:public CxxTest::TestSuite
{
    public:
        PADnote      *note;
        PADnoteParameters *pars;
        Master       *master;
        FFTwrapper   *fft;
        Controller   *controller;
        AbsTime      *time;
        float test_freq_log2;
        Alloc         memory;
        int           interpolation;
        rtosc::ThreadLink *tr;
        WatchManager *w;


        float *outR, *outL;

        void setUp() {
            interpolation = 0;
            synth = new SYNTH_T;
            //First the sensible settings and variables that have to be set:
            synth->buffersize = 256;
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
            pars = new PADnoteParameters(*synth, fft, time);


            //Assert defaults
            ///TS_ASSERT(!defaultPreset->VoicePar[1].Enabled);

            XMLwrapper wrap;
            cout << string(SOURCE_DIR) + string("/guitar-adnote.xmz")
                 << endl;
            wrap.loadXMLfile(string(SOURCE_DIR)
                              + string("/guitar-adnote.xmz"));
            TS_ASSERT(wrap.enterbranch("MASTER"));
            TS_ASSERT(wrap.enterbranch("PART", 2));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT"));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT_KIT"));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT_KIT_ITEM", 0));
            TS_ASSERT(wrap.enterbranch("PAD_SYNTH_PARAMETERS"));
            pars->getfromXML(wrap);


            //defaultPreset->defaults();
            pars->applyparameters([]{return false;}, 1);

            //verify xml was loaded
            ///TS_ASSERT(defaultPreset->VoicePar[1].Enabled);



            controller = new Controller(*synth, time);

            //lets go with.... 50! as a nice note
            test_freq_log2 = log2f(440.0f) + (50.0 - 69.0f) / 12.0f;
            SynthParams pars_{memory, *controller, *synth, *time, 120, 0, test_freq_log2, false, prng()};

            note = new PADnote(pars, pars_, interpolation);
        }

        void tearDown() {
            delete note;
            delete controller;
            delete fft;
            delete [] outL;
            delete [] outR;
            delete pars;
            FFT_cleanup();
            delete synth;

            note = NULL;
            controller = NULL;
            fft = NULL;
            outL = NULL;
            outR = NULL;
            pars = NULL;
            synth = NULL;
        }


        void testDefaults() {
            int sampleCount = 0;


//#define WRITE_OUTPUT

#ifdef WRITE_OUTPUT
            ofstream file("padnoteout", ios::out);
#endif
            note->noteout(outL, outR);

#ifdef WRITE_OUTPUT
            for(int i = 0; i < synth->buffersize; ++i)
                file << outL[i] << std::endl;

#endif
            sampleCount += synth->buffersize;

            TS_ASSERT_DELTA(outL[255], 0.3950, 0.0005f);


            note->releasekey();

            TS_ASSERT(!tr->hasNext());
            w->add_watch("noteout");
            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], -0.2305f, 0.0005f);
            w->tick();
            TS_ASSERT(!tr->hasNext());

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], -0.1164f, 0.0005f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], 0.1079, 0.0005f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], 0.0841f, 0.0001f);

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

            TS_ASSERT_EQUALS(sampleCount, 5888);
        }

        void testInitialization() {
            TS_ASSERT_EQUALS(pars->Pmode, PADnoteParameters::pad_mode::bandwidth);

            TS_ASSERT_EQUALS(pars->PVolume, 90);
            TS_ASSERT(pars->oscilgen);
            TS_ASSERT(pars->resonance);

            TS_ASSERT_DELTA(note->NoteGlobalPar.Volume, 2.597527f, 0.001f);
            TS_ASSERT_DELTA(note->NoteGlobalPar.Panning, 0.500000f, 0.01f);


            for(int i=0; i<8; ++i)
                TS_ASSERT(pars->sample[i].smp);
            for(int i=8; i<PAD_MAX_SAMPLES; ++i)
                TS_ASSERT(!pars->sample[i].smp);

            TS_ASSERT_DELTA(pars->sample[0].smp[0],   0.0516f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[1],   0.0845f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[2],   0.1021f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[3],   0.0919f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[4],   0.0708f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[5],   0.0414f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[6],   0.0318f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[7],   0.0217f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[8],   0.0309f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[9],   0.0584f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[10],  0.0266f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[11],  0.0436f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[12],  0.0199f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[13],  0.0505f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[14],  0.0438f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[15],  0.0024f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[16],  0.0052f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[17], -0.0180f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[18],  0.0342f, 0.0005f);
            TS_ASSERT_DELTA(pars->sample[0].smp[19],  0.0051f, 0.0005f);


            //Verify Harmonic Input
            float harmonics[synth->oscilsize];
            memset(harmonics, 0, sizeof(float) * synth->oscilsize);

            pars->oscilgen->get(harmonics, 440.0f, false);

            TS_ASSERT_DELTA(harmonics[0] ,0.683947, 0.0005f);
            TS_ASSERT_DELTA(harmonics[1] ,0.128246, 0.0005f);
            TS_ASSERT_DELTA(harmonics[2] ,0.003238, 0.0005f);
            TS_ASSERT_DELTA(harmonics[3] ,0.280945, 0.0005f);
            TS_ASSERT_DELTA(harmonics[4] ,0.263548, 0.0005f);
            TS_ASSERT_DELTA(harmonics[5] ,0.357070, 0.0005f);
            TS_ASSERT_DELTA(harmonics[6] ,0.096287, 0.0005f);
            TS_ASSERT_DELTA(harmonics[7] ,0.128685, 0.0005f);
            TS_ASSERT_DELTA(harmonics[8] ,0.003238, 0.0005f);
            TS_ASSERT_DELTA(harmonics[9] ,0.149376, 0.0005f);
            TS_ASSERT_DELTA(harmonics[10],0.063892, 0.0005f);
            TS_ASSERT_DELTA(harmonics[11],0.296716, 0.0005f);
            TS_ASSERT_DELTA(harmonics[12],0.051057, 0.0005f);
            TS_ASSERT_DELTA(harmonics[13],0.066310, 0.0005f);
            TS_ASSERT_DELTA(harmonics[14],0.004006, 0.0005f);
            TS_ASSERT_DELTA(harmonics[15],0.038662, 0.0005f);

            float sum = 0;
            for(int i=0; i<synth->oscilsize/2; ++i)
                sum += harmonics[i];
            TS_ASSERT_DELTA(sum, 5.863001, 0.0005f);

            TS_ASSERT_DELTA(pars->getNhr(0), 0.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(1), 1.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(2), 2.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(3), 3.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(4), 4.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(5), 5.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(6), 6.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(7), 7.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(8), 8.000000, 0.0005f);
            TS_ASSERT_DELTA(pars->getNhr(9), 9.000000, 0.0005f);

        }

#define OUTPUT_PROFILE
#ifdef OUTPUT_PROFILE
        void testSpeed() {
            const int samps = 15000;

            int t_on = clock(); // timer before calling func
            for(int i = 0; i < samps; ++i)
                note->noteout(outL, outR);
            int t_off = clock(); // timer when func returns

            printf("PadNoteTest: %f seconds for %d Samples to be generated.\n",
                   (static_cast<float>(t_off - t_on)) / CLOCKS_PER_SEC, samps);
        }
#endif
};
