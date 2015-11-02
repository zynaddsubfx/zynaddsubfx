/*
  ZynAddSubFX - a software synthesizer

  PluginTest.h - CxxTest for embedding zyn
  Copyright (C) 2013-2013 Mark McCurry
  Authors: Mark McCurry

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
#include <fstream>
#include <string>
#include "../Misc/MiddleWare.h"
#include "../Misc/Master.h"
#include "../Misc/PresetExtractor.h"
#include "../Misc/PresetExtractor.cpp"
#include "../Misc/Util.h"
#include "../globals.h"
#include "../UI/NSM.H"
NSM_Client *nsm = 0;
MiddleWare *middleware = 0;

using namespace std;

char *instance_name=(char*)"";

#define NUM_MIDDLEWARE 3

class PluginTest:public CxxTest::TestSuite
{
    public:
        Config config;
        void setUp() {
            synth = new SYNTH_T;
            synth->buffersize = 256;
            synth->samplerate = 48000;
            //synth->alias();

            outL  = new float[1024];
            for(int i = 0; i < synth->buffersize; ++i)
                outL[i] = 0.0f;
            outR = new float[1024];
            for(int i = 0; i < synth->buffersize; ++i)
                outR[i] = 0.0f;

            delete synth;
            synth = NULL;
            for(int i = 0; i < NUM_MIDDLEWARE; ++i) {
                synth = new SYNTH_T;
                synth->buffersize = 256;
                synth->samplerate = 48000;
                //synth->alias();
                middleware[i] = new MiddleWare(std::move(*synth), &config);
                master[i] = middleware[i]->spawnMaster();
                //printf("Octave size = %d\n", master[i]->microtonal.getoctavesize());
            }
        }

        void tearDown() {
            for(int i = 0; i < NUM_MIDDLEWARE; ++i)
                delete middleware[i];

            delete[] outL;
            delete[] outR;
            delete synth;
        }


        void testInit() {

            for(int x=0; x<100; ++x) {
                for(int i=0; i<NUM_MIDDLEWARE; ++i) {
                    middleware[i]->tick();
                    master[i]->GetAudioOutSamples(rand()%1025,
                            synth->samplerate, outL, outR);
                }
            }
        }

        void testPanic()
        {
            master[0]->setController(0, 0x64, 0);
            master[0]->noteOn(0,64,64);
            master[0]->AudioOut(outL, outR);

            float sum = 0.0f;
            for(int i = 0; i < synth->buffersize; ++i)
                sum += fabs(outL[i]);

            TS_ASSERT_LESS_THAN(0.1f, sum);
        }

        string loadfile(string fname) const
        {
            std::ifstream t(fname.c_str());
            std::string str((std::istreambuf_iterator<char>(t)),
                                     std::istreambuf_iterator<char>());
            return str;
        }

        void testLoad(void)
        {
            for(int i=0; i<NUM_MIDDLEWARE; ++i) {
                middleware[i]->transmitMsg("/load-part", "is", 0, (string(SOURCE_DIR) + "/../../instruments/banks/Organ/0037-Church Organ 1.xiz").c_str());
                middleware[i]->tick();
                master[i]->GetAudioOutSamples(synth->buffersize, synth->samplerate, outL, outR);
                middleware[i]->tick();
                master[i]->GetAudioOutSamples(synth->buffersize, synth->samplerate, outL, outR);
                middleware[i]->tick();
                master[i]->GetAudioOutSamples(synth->buffersize, synth->samplerate, outL, outR);
                middleware[i]->tick();
                master[i]->GetAudioOutSamples(synth->buffersize, synth->samplerate, outL, outR);
                middleware[i]->tick();
            }
            //const string fname = string(SOURCE_DIR) + "/../../instruments/banks/Organ/0037-Church Organ 1.xiz";
            //const string fdata = loadfile(fname);
        }


    private:
        SYNTH_T *synth;
        float *outR, *outL;
        MiddleWare *middleware[NUM_MIDDLEWARE];
        Master *master[NUM_MIDDLEWARE];
};
