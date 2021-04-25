/*
  ZynAddSubFX - a software synthesizer

  PluginTest.h - CxxTest for embedding zyn
  Copyright (C) 2013-2013 Mark McCurry
  Authors: Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "test-suite.h"
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
using namespace std;
using namespace zyn;

NSM_Client *nsm = 0;
MiddleWare *middleware = 0;

char *instance_name=(char*)"";

#define NUM_MIDDLEWARE 3

class PluginTest
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
                sum += fabsf(outL[i]);

            TS_ASSERT(0.1f < sum);
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

        void testChangeToOutOfRangeProgram()
        {
            middleware[0]->transmitMsg("/bank/msb", "i", 0);
            middleware[0]->tick();
            middleware[0]->transmitMsg("/bank/lsb", "i", 1);
            middleware[0]->tick();
            middleware[0]->pendingSetProgram(0, 32);
            middleware[0]->tick();
            master[0]->GetAudioOutSamples(synth->buffersize, synth->samplerate, outL, outR);
            // We should ideally be checking to verify that the part change
            // didn't happen, but it's not clear how to do that.  We're
            // currently relying on the assert(filename) in loadPart() failing
            // if this logic gets broken.
        }

    private:
        SYNTH_T *synth;
        float *outR, *outL;
        MiddleWare *middleware[NUM_MIDDLEWARE];
        Master *master[NUM_MIDDLEWARE];
};

int main()
{
    PluginTest test;
    RUN_TEST(testInit);
    RUN_TEST(testPanic);
    RUN_TEST(testLoad);
    RUN_TEST(testChangeToOutOfRangeProgram);
    return test_summary();
}
