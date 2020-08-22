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
#include "../Misc/Util.h"
#include "../Misc/Allocator.h"
#include "../Synth/ADnote.h"
#include "../Synth/OscilGen.h"
#include "../Params/Presets.h"
#include "../Params/FilterParams.h"
#include "../DSP/FFTwrapper.h"
#include "../globals.h"
using namespace std;
using namespace zyn;

SYNTH_T *synth;

#define BUF 256
class AdNoteTest:public CxxTest::TestSuite
{
    public:

        ADnote       *note;
        FFTwrapper   *fft;
        Controller   *controller;
        float test_freq_log2;
        ADnoteParameters *params;
        AbsTime  *time;
        Alloc     memory;


        float outR[BUF], outL[BUF];

        void setUp() {
            //First the sensible settings and variables that have to be set:
            synth = new SYNTH_T;
            synth->buffersize = BUF;
            synth->alias();
            time = new AbsTime(*synth);

            memset(outL,0,sizeof(outL));
            memset(outR,0,sizeof(outR));

            fft = new FFTwrapper(BUF);
            //prepare the default settings
            params = new ADnoteParameters(*synth, fft, time);

            //sawtooth to make things a bit more interesting
            params->VoicePar[0].OscilGn->Pcurrentbasefunc = 3;

            params->GlobalPar.PFilterVelocityScale = 64;
            params->GlobalPar.GlobalFilter->basefreq = 5076.203125;

            controller = new Controller(*synth, time);

            //lets go with.... 50! as a nice note
            test_freq_log2 = log2f(440.0f) + (50.0 - 69.0f) / 12.0f;
        }

        void tearDown() {
            delete note;
            delete controller;
            delete fft;
            FFT_cleanup();
            delete time;
            delete synth;
            delete params;
        }

        void run_test(int a, int b, int c, int d, int e, int f, float values[4])
        {
            sprng(0);

            const int ADnote_unison_sizes[] =
            {1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30, 40, 50, 0};
            params->VoicePar[0].Unison_size = ADnote_unison_sizes[a];
            params->VoicePar[0].Unison_frequency_spread = b;
            params->VoicePar[0].Unison_stereo_spread    = c;
            params->VoicePar[0].Unison_vibratto         = d;
            params->VoicePar[0].Unison_vibratto_speed   = e;
            params->VoicePar[0].Unison_invert_phase     = f;

            SynthParams pars{memory, *controller, *synth, *time, 120, 0, test_freq_log2, false, prng()};
            note = new ADnote(params, pars);
            note->noteout(outL, outR);
            TS_ASSERT_DELTA(outL[80], values[0], 1.9e-5);
            printf("\n{%f,", outL[80]);
            note->noteout(outL, outR);
            TS_ASSERT_DELTA(outR[90], values[1], 1.9e-5);
            printf("\n%f,", outR[90]);
            note->noteout(outL, outR);
            TS_ASSERT_DELTA(outL[20], values[2], 1.9e-5);
            printf("\n%f,", outL[20]);
            note->noteout(outL, outR);
            TS_ASSERT_DELTA(outR[200], values[3], 1.9e-5);
            printf("\n%f},\n", outR[200]);
        }

        void testUnison() {
            sprng(0xbeef);

            float data[][4] = {
                {-0.034547,0.034349,-0.000000,0.138284},
                {0.016801,-0.084991,0.000000,0.009240},
                {0.020383,-0.002424,-0.012952,-0.014037},
                {-0.041653,0.002287,0.000000,-0.098181},
                {-0.009189,-0.049860,0.000268,-0.084961},
                {0.056999,-0.084627,-0.018144,0.000666},
                {-0.015588,0.003690,0.003994,0.002435},
                {0.023178,-0.024961,0.004433,-0.015144},
                {0.042007,-0.006559,-0.005887,0.083685},
                {0.007638,0.057870,-0.014244,0.041457},
                {-0.018006,-0.017846,-0.063624,-0.016378},
                {0.004914,-0.001756,-0.046715,0.015975},
                {0.004341,-0.014575,0.000560,0.050902},
                {0.000470,-0.036961,0.038622,0.031383},
                {-0.045796,0.000262,0.009858,0.031958},
            };

            int freq_spread[15];
            int stereo_spread[15];
            int vibrato[15];
            int vibrato_speed[15];
            int inv_phase[15];
            for(int i=0; i<15; ++i)
            {
                freq_spread[i]    = prng()%0x7f;
                stereo_spread[i]  = prng()%0x7f;
                vibrato[i]        = prng()%0x7f;
                vibrato_speed[i]  = prng()%0x7f;
                inv_phase[i]      = prng()%5;
            }

            for(int i=0; i<15; ++i)
            {
                run_test(i, freq_spread[i], stereo_spread[i],
                        vibrato[i], vibrato_speed[i], inv_phase[i], data[i]);
            }
#if 0
            int sampleCount = 0;

            sampleCount += synth->buffersize;

            TS_ASSERT_DELTA(outL[255], 0.254609f, 0.0001f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], -0.102197f, 0.0001f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], -0.111422f, 0.0001f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], -0.021375f, 0.0001f);

            note->noteout(outL, outR);
            sampleCount += synth->buffersize;
            TS_ASSERT_DELTA(outL[255], 0.149882f, 0.0001f);
#endif
        }
};
