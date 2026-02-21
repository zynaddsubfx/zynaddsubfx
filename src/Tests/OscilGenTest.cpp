/*
  ZynAddSubFX - a software synthesizer

  AdNoteTest.h - CxxTest for Synth/OscilGen
  Copyright (C) 20011-2012 Mark McCurry
  Author: Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "test-suite.h"
#include <string>
#include "../Synth/OscilGen.h"
#include "../Misc/XMLwrapper.h"
#include "../DSP/FFTwrapper.h"
#include "../Misc/Util.h"
#include "../globals.h"
using namespace std;
using namespace zyn;

SYNTH_T *synth;

class OscilGenTest
{
    public:
        float  freq;
        float *outR, *outL;
        FFTwrapper *fft;
        OscilGen   *oscil;

        void setUp() {
            synth = new SYNTH_T;
            //First the sensible settings and variables that have to be set:
            synth->buffersize = 256;
            synth->oscilsize  = 1024;

            outL = new float[synth->oscilsize];
            outR = new float[synth->oscilsize];
            memset(outL, 0, sizeof(float) * synth->oscilsize);
            memset(outR, 0, sizeof(float) * synth->oscilsize);

            //prepare the default settings
            fft   = new FFTwrapper(synth->oscilsize);
            oscil = new OscilGen(*synth, fft, NULL);

            //Assert defaults [TODO]


            XMLwrapper wrap;
            wrap.loadXMLfile(string(SOURCE_DIR)
                              + string("/guitar-adnote.xmz"));
            TS_ASSERT(wrap.enterbranch("MASTER"));
            TS_ASSERT(wrap.enterbranch("PART", 0));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT"));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT_KIT"));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT_KIT_ITEM", 0));
            TS_ASSERT(wrap.enterbranch("ADD_SYNTH_PARAMETERS"));
            TS_ASSERT(wrap.enterbranch("VOICE", 0));
            TS_ASSERT(wrap.enterbranch("OSCIL"));
            oscil->getfromXML(wrap);

            //verify xml was loaded [TODO]

            //lets go with.... 50! as a nice note
            const char testnote = 50;
            freq = 440.0f * powf(2.0f, (testnote - 69.0f) / 12.0f);
        }

        void tearDown() {
            delete oscil;
            delete fft;
            delete[] outL;
            delete[] outR;
            FFT_cleanup();
            delete synth;
        }

        //verifies that initialization occurs
        void testInit(void)
        {
            oscil->get(outL, freq);
        }

        void testOutput(void)
        {
            oscil->get(outL, freq);
            TS_ASSERT_DELTA(outL[23], -0.5371f, 0.0001f);
            TS_ASSERT_DELTA(outL[129], 0.3613f, 0.0001f);
            TS_ASSERT_DELTA(outL[586], 0.1118f, 0.0001f);
            TS_ASSERT_DELTA(outL[1023], -0.6889f, 0.0001f);
        }

        void testSpectrum(void)
        {
            oscil->getspectrum(synth->oscilsize / 2, outR, 1);
            TS_ASSERT_DELTA(outR[1], 350.698059f, 0.0001f);
            TS_ASSERT_DELTA(outR[2], 228.889267f, 0.0001f);
            TS_ASSERT_DELTA(outR[3], 62.187931f, 0.0001f);
            TS_ASSERT_DELTA(outR[4], 22.295225f, 0.0001f);
            TS_ASSERT_DELTA(outR[5], 6.942001f, 0.0001f);
            TS_ASSERT_DELTA(outR[27], 0.015110f, 0.0001f);
            TS_ASSERT_DELTA(outR[48], 0.003425f, 0.0001f);
            TS_ASSERT_DELTA(outR[66], 0.001293f, 0.0001f);
        }

        //performance testing
#ifdef __linux__
        void testSpeed() {
            const int samps = 15000;

            int t_on = clock(); // timer before calling func
            for(int i = 0; i < samps; ++i)
                oscil->prepare();
            int t_off = clock(); // timer when func returns

            printf("OscilGenTest: %f seconds for %d prepares.\n",
                   (static_cast<float>(t_off - t_on)) / CLOCKS_PER_SEC, samps);

            t_on = clock(); // timer before calling func
            for(int i = 0; i < samps; ++i)
                oscil->get(outL, freq);
            t_off = clock(); // timer when func returns

            printf("OscilGenTest: %f seconds for %d gets.\n",
                   (static_cast<float>(t_off - t_on)) / CLOCKS_PER_SEC, samps);
        }
#endif
};

int main()
{
    OscilGenTest test;
    RUN_TEST(testInit);
    RUN_TEST(testOutput);
    RUN_TEST(testSpectrum);
#ifdef __linux__
    RUN_TEST(testSpeed);
#endif
    return test_summary();
}
