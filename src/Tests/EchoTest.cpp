/*
  ZynAddSubFX - a software synthesizer

  EchoTest.h - CxxTest for Effect/Echo
  Copyright (C) 2009-2011 Mark McCurry
  Copyright (C) 2009 Harald Hvaal
  Authors: Mark McCurry, Harald Hvaal

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "test-suite.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include "../Effects/Echo.h"
#include "../Misc/Allocator.h"
#include "../globals.h"

using namespace std;
using namespace zyn;

SYNTH_T *synth;

class EchoTest
{
    public:
        void setUp() {
            synth = new SYNTH_T;
            outL  = new float[synth->buffersize];
            for(int i = 0; i < synth->buffersize; ++i)
                outL[i] = 0.0f;
            outR = new float[synth->buffersize];
            for(int i = 0; i < synth->buffersize; ++i)
                outR[i] = 0.0f;
            input = new Stereo<float *>(new float[synth->buffersize],
                                        new float[synth->buffersize]);
            for(int i = 0; i < synth->buffersize; ++i)
                input->l[i] = input->r[i] = 0.0f;
            EffectParams pars{alloc,true, outL, outR, 0, 44100, 256, nullptr};
            testFX = new Echo(pars);
        }

        void tearDown() {
            delete[] input->r;
            delete[] input->l;
            delete input;
            delete[] outL;
            delete[] outR;
            delete testFX;
            delete synth;
        }


        void testInit() {
            //Make sure that the output will be zero at start
            //(given a zero input)
            testFX->out(*input);
            for(int i = 0; i < synth->buffersize; ++i) {
                TS_ASSERT_DELTA(outL[i], 0.0f, 0.0001f);
                TS_ASSERT_DELTA(outR[i], 0.0f, 0.0001f);
            }
        }

        void testClear() {
            char DELAY = 2;
            testFX->changepar(DELAY, 127);

            //flood with high input
            for(int i = 0; i < synth->buffersize; ++i)
                input->r[i] = input->l[i] = 1.0f;

            for(int i = 0; i < 500; ++i)
                testFX->out(*input);
            for(int i = 0; i < synth->buffersize; ++i) {
                TS_ASSERT(outL[i] != 0.0f);
                TS_ASSERT(outR[i] != 0.0f);
            }
            //After making sure the internal buffer has a nonzero value
            //cleanup
            //Then get the next output, which should be zereoed out if DELAY
            //is large enough
            testFX->cleanup();
            testFX->out(*input);
            for(int i = 0; i < synth->buffersize; ++i) {
                TS_ASSERT_DELTA(outL[i], 0.0f, 0.0001f);
                TS_ASSERT_DELTA(outR[i], 0.0f, 0.0001f);
            }
        }
        //Insures that the proper decay occurs with high feedback
        void testDecaywFb() {
            //flood with high input
            for(int i = 0; i < synth->buffersize; ++i)
                input->r[i] = input->l[i] = 1.0f;
            char FEEDBACK = 5;
            testFX->changepar(FEEDBACK, 127);
            for(int i = 0; i < 100; ++i)
                testFX->out(*input);
            for(int i = 0; i < synth->buffersize; ++i) {
                TS_ASSERT(outL[i] != 0.0f);
                TS_ASSERT(outR[i] != 0.0f);
            }
            float amp = abs(outL[0] + outR[0]) / 2;
            //reset input to zero
            for(int i = 0; i < synth->buffersize; ++i)
                input->r[i] = input->l[i] = 0.0f;

            //give the echo time to fade based upon zero input and high feedback
            for(int i = 0; i < 50; ++i)
                testFX->out(*input);
            TS_ASSERT(abs(outL[0] + outR[0]) / 2 <= amp);
        }


    private:
        Stereo<float *> *input;
        float *outR, *outL;
        Echo  *testFX;
        Alloc alloc;
};

int main()
{
    tap_quiet = 1;
    EchoTest test;
    RUN_TEST(testInit);
    RUN_TEST(testClear);
    RUN_TEST(testDecaywFb);
    return test_summary();
}
