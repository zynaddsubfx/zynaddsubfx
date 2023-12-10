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


#include "test-suite.h"
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
class UnisonTest
{
    public:

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

            fft = new FFTwrapper(synth->oscilsize);
            //prepare the default settings
            params = new ADnoteParameters(*synth, fft, time, false);

            //sawtooth to make things a bit more interesting
            params->VoicePar[0].OscilGn->Pcurrentbasefunc = 3;

            params->GlobalPar.PFilterVelocityScale = 64;
            params->GlobalPar.GlobalFilter->basefreq = 5076.203125;

            controller = new Controller(*synth, time);

            //lets go with.... 50! as a nice note
            test_freq_log2 = log2f(440.0f) + (50.0 - 69.0f) / 12.0f;
        }

        void tearDown() {
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
            ADnote* note = new ADnote(params, pars);
            note->noteout(outL, outR);
            TS_ASSERT_DELTA(values[0], outL[80], 1.9e-5);
            printf("{%f,", outL[80]);
            note->noteout(outL, outR);
            TS_ASSERT_DELTA(values[1], outR[90], 1.9e-5);
            printf("%f,", outR[90]);
            note->noteout(outL, outR);
            TS_ASSERT_DELTA(values[2], outL[20], 1.9e-5);
            printf("%f,", outL[20]);
            note->noteout(outL, outR);
            TS_ASSERT_DELTA(values[3], outR[200], 1.9e-5);
            printf("%f},\n", outR[200]);
            delete note;
        }

        void testUnison() {
            sprng(0xbeef);

            float data[][4] = {
                {0.009012,-0.074734,-0.012636,-0.022757},
                {-0.004497,-0.106839,-0.003898,-0.060385},
                {-0.027254,-0.048478,0.019716,-0.036242},
                {0.013156,-0.104055,-0.000360,-0.152481},
                {0.031217,-0.125947,-0.035797,-0.083334},
                {0.061165,0.111355,-0.044391,0.065363},
                {-0.005528,-0.063873,0.008229,0.001122},
                {0.070588,0.092495,-0.081852,0.104497},
                {0.065521,0.052868,-0.066022,0.110135},
                {0.094440,-0.028949,-0.096088,-0.002352},
                {0.072197,0.040330,-0.102320,0.048607},
                {-0.045416,0.106115,0.059613,0.041986},
                {0.069641,-0.061826,-0.082194,0.005864},
                {-0.035529,-0.145869,0.033578,-0.124722},
                {0.075097,0.027494,-0.075664,0.046463},
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

            printf("\n");
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

int main()
{
    UnisonTest test;
    RUN_TEST(testUnison);
    return test_summary();
}
