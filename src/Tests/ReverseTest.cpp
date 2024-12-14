/*
  ZynAddSubFX - a software synthesizer

  ReverseTest.h - CxxTest for Effect/Reverse
  Copyright (C) 2024 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "test-suite.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include "../Effects/Reverse.h"
#include "../Misc/Allocator.h"
#include "../Misc/Time.h"
#include "../globals.h"

using namespace std;
using namespace zyn;

SYNTH_T *synth;

class ReverseTest
{
    public:
        void setUp() {
            synth = new SYNTH_T;
            time  = new AbsTime(*synth);
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
            testFX = new Reverse(pars,time);
        }

        void tearDown() {
            delete[] input->r;
            delete[] input->l;
            delete input;
            delete[] outL;
            delete[] outR;
            delete testFX;
            delete time;
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
            char MODE = 6;
            testFX->changepar(DELAY, 127);
            testFX->changepar(MODE, 2);

            //flood with high input
            for(int i = 0; i < synth->buffersize; ++i)
                input->r[i] = input->l[i] = 1.0f;

            for(int i = 0; i < 5000; ++i)
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
        void testRandom() {
            const int steps = 100000;
            for(int i = 0; i < steps; ++i) {

                //input is [-0.5..0.5]
                for(int j = 0; j < synth->buffersize; ++j)
                    input->r[j] = input->l[j] = RND-0.5;

                for(int j = 0; j < 6; ++j) {
                    if(RND < 0.01) {//1% chance a paramter change occurs
                        int value = prng()%128;
                        if(j == 6)
                            value = prng()%8;
                        testFX->changepar(j, value);
                    }
                }

                testFX->out(*input);

                for(int i = 0; i < synth->buffersize; ++i) {
                    TS_ASSERT(fabsf(outL[i]) < 0.75);
                    TS_ASSERT(fabsf(outR[i]) < 0.75);
                }
            }
        }


    private:
        Stereo<float *> *input;
        float *outR, *outL;
        AbsTime  *time;
        Reverse  *testFX;
        Alloc alloc;
};

int main()
{
    tap_quiet = 1;
    ReverseTest test;
    RUN_TEST(testInit);
    RUN_TEST(testClear);
    RUN_TEST(testRandom);
    return test_summary();
}
