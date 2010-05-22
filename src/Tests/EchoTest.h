/*
  ZynAddSubFX - a software synthesizer

  EchoTest.h - CxxTest for Effect/Echo
  Copyright (C) 2009-2009 Mark McCurry
  Author: Mark McCurry

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/
#include <cxxtest/TestSuite.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include "../Effects/Echo.h"
#include "../globals.h"

using namespace std;

class EchoTest:public CxxTest::TestSuite
{
    public:
        void setUp() {
            outL = new float[SOUND_BUFFER_SIZE];
            for(int i = 0; i < SOUND_BUFFER_SIZE; ++i)
                outL[i] = 0.0;
            outR = new float[SOUND_BUFFER_SIZE];
            for(int i = 0; i < SOUND_BUFFER_SIZE; ++i)
                outR[i] = 0.0;
            input  = new Stereo<REALTYPE *>(new REALTYPE[SOUND_BUFFER_SIZE],new REALTYPE[SOUND_BUFFER_SIZE]);
            for(int i = 0; i < SOUND_BUFFER_SIZE; ++i)
                input->l()[i] = input->r()[i] = 0.0f;
            testFX = new Echo(true, outL, outR);
        }

        void tearDown() {
            delete[] input->r();
            delete[] input->l();
            delete input;
            delete[] outL;
            delete[] outR;
            delete testFX;
        }


        void testInit() {
            //Make sure that the output will be zero at start
            //(given a zero input)
            testFX->out(*input);
            for(int i = 0; i < SOUND_BUFFER_SIZE; ++i) {
                TS_ASSERT_DELTA(outL[i], 0.0, 0.0001);
                TS_ASSERT_DELTA(outR[i], 0.0, 0.0001);
            }
        }

        void testClear() {
            char DELAY = 2;
            testFX->changepar(DELAY, 127);

            //flood with high input
            for(int i = 0; i < SOUND_BUFFER_SIZE; ++i)
                input->r()[i] = input->l()[i] = 1.0;

            for(int i = 0; i < 500; ++i)
                testFX->out(*input);
            for(int i = 0; i < SOUND_BUFFER_SIZE; ++i) {
                TS_ASSERT_DIFFERS(outL[i], 0.0);
                TS_ASSERT_DIFFERS(outR[i], 0.0)
            }
            //After making sure the internal buffer has a nonzero value
            //cleanup
            //Then get the next output, which should be zereoed out if DELAY
            //is large enough
            testFX->cleanup();
            testFX->out(*input);
            for(int i = 0; i < SOUND_BUFFER_SIZE; ++i) {
                TS_ASSERT_DELTA(outL[i], 0.0, 0.0001);
                TS_ASSERT_DELTA(outR[i], 0.0, 0.0001);
            }
        }
        //Insures that the proper decay occurs with high feedback
        void testDecaywFb() {

            //flood with high input
            for(int i = 0; i < SOUND_BUFFER_SIZE; ++i)
                input->r()[i] = input->l()[i] = 1.0;
            char FEEDBACK = 5;
            testFX->changepar(FEEDBACK, 127);
            for(int i = 0; i < 100; ++i)
                testFX->out(*input);
            for(int i = 0; i < SOUND_BUFFER_SIZE; ++i) {
                TS_ASSERT_DIFFERS(outL[i], 0.0);
                TS_ASSERT_DIFFERS(outR[i], 0.0)
            }
            float amp = abs(outL[0] + outR[0]) / 2;
            //reset input to zero
            for(int i = 0; i < SOUND_BUFFER_SIZE; ++i)
                input->r()[i] = input->l()[i] = 0.0;

            //give the echo time to fade based upon zero input and high feedback
            for(int i = 0; i < 50; ++i)
                testFX->out(*input);
            TS_ASSERT_LESS_THAN_EQUALS(abs(outL[0] + outR[0]) / 2, amp);
        }


    private:
        Stereo<REALTYPE *> *input;
        float *outR, *outL;
        Echo  *testFX;
};

