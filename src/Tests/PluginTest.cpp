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

SYNTH_T *synth;
NSM_Client *nsm = 0;

char *instance_name=(char*)"";
MiddleWare *middleware;

void fill_vec_with_lines(std::vector<string> &v, string s)
{
    std::istringstream stream(s);
    std::string line;
    while(std::getline(stream, line))
        v.push_back(line);
}
void print_string_differences(string orig, string next)
{
    std::vector<string> a, b;
    fill_vec_with_lines(a, orig);
    fill_vec_with_lines(b, next);
    int N = a.size();
    int M = b.size();
    printf("%d vs %d\n", N, M);

    //Original String by New String
    //Each step is either an insertion, deletion, or match
    //Match     is 0 cost and moves +1 State (if symbols are the same)
    //Replace   is 3 cost and moves +1 State (if symbols are different)
    //Insertion is 2 cost and moves +2 State (+2 if symbols are different)
    //Deletion  is 1 cost and moves +0 State (+2 if symbols are different)
    char *transition = new char[N*M];
    float  *cost       = new float[N*M];

    const int match  = 1;
    const int insert = 2;
    const int del    = 3;
    for(int i=0; i<N; ++i) {
        for(int j=0; j<M; ++j) {
            transition[i*M + j] = 0;
            cost[i*M + j] = 0xffff;
        }
    }

    //Just assume the -1 line is the same
    cost[0*M + 0] = (a[0] != b[0])*3;
    cost[0*M + 1] = (a[1] != b[0])*2 + 2;
    for(int i=1; i<N; ++i) {
        for(int j=0; j<M; ++j) {
            int cost_match = 0xffffff;
            int cost_ins   = 0xffffff;
            int cost_del   = 0xffffff;
            cost_del = cost[(i-1)*M + j] + 2 + (a[i] != b[j])*2;
            if(j > 1)
                cost_ins = cost[(i-1)*M + (j-2)] + 1 + 2*(a[i] != b[j]);
            if(j > 0)
                cost_match = cost[(i-1)*M + (j-1)] + 3*(a[i] != b[j]);

            if(cost_match >= 0xffff && cost_ins >= 0xffff && cost_del >= 0xffff) {
                ;
            } else if(cost_match < cost_ins && cost_match < cost_del) {
                cost[i*M+j] = cost_match;
                transition[i*M+j] = match;
            } else if(cost_ins < cost_del) {
                cost[i*M+j] = cost_ins;
                transition[i*M+j] = insert;
            } else {
                cost[i*M+j] = cost_del;
                transition[i*M+j] = del;
            }
        }
    }

    //int off = 0;
    //int len = N;
    //for(int i=off; i<off+len; ++i) {
    //    for(int j=off; j<off+len; ++j) {
    //        printf("%4d ", (int)(cost[i*M+j] > 4000 ? -1 : cost[i*M+j]));
    //    }
    //    printf("\n");
    //}
    //for(int i=off; i<off+len; ++i) {
    //    for(int j=off; j<off+len; ++j) {
    //        printf("%d ", transition[i*M+j]);
    //    }
    //    printf("\n");
    //}

    //for(int i=off; i<off+len; ++i)
    //    printf("%d: %s\n", i, a[i].c_str());
    //for(int i=off; i<off+len; ++i)
    //    printf("%d: %s\n", i, b[i].c_str());
    //exit(1);

    int total_cost = cost[(N-1)*M + (M-1)];
    if(total_cost < 500) {
        printf("total cost = %f\n", cost[(N-1)*M + (M-1)]);

        int b_pos = b.size()-1;
        int a_pos = a.size()-1;
        while(a_pos > 0 && b_pos > 0) {
            //printf("state = (%d, %d) => %f\n", a_pos, b_pos, cost[a_pos*M+b_pos]);
            if(transition[a_pos*M+b_pos] == match) {
                if(a[a_pos] != b[b_pos]) {
                    printf("REF - %s\n", a[a_pos].c_str());
                    printf("NEW + %s\n", b[b_pos].c_str());
                }
                //printf("R");
                a_pos -= 1;
                b_pos -= 1;
            } else if(transition[a_pos*M+b_pos] == del) {
                //printf("D");
                //if(a[a_pos] != b[b_pos]) {
                    //printf("- %s\n", a[a_pos].c_str());
                    printf("NEW - %s\n", b[b_pos].c_str());
                //}
                b_pos -= 1;
            } else if(transition[a_pos*M+b_pos] == insert) {
                //if(a[a_pos] != b[b_pos]) {
                printf("REF - %s\n", a[a_pos].c_str());
                printf("NEW + %s\n", b[b_pos].c_str());
                printf("NEW + %s\n", b[b_pos-1].c_str());
                //}
                //printf("I");
                a_pos -= 1;
                b_pos -= 2;
            } else {
                printf("ERROR STATE @(%d, %d)\n", a_pos, b_pos);
                exit(1);
            }

        }
        //printf("%d vs %d\n", N, M);
    } else {
        printf("[WARNING] XML File appears to be radically different\n");
    }
}

class PluginTest
{
    public:
        Config config;
        void setUp() {
            synth = new SYNTH_T;
            synth->buffersize = 256;
            synth->samplerate = 48000;
            synth->alias();

            outL  = new float[1024];
            for(int i = 0; i < synth->buffersize; ++i)
                outL[i] = 0.0f;
            outR = new float[1024];
            for(int i = 0; i < synth->buffersize; ++i)
                outR[i] = 0.0f;

            for(int i = 0; i < 16; ++i)
                master[i] = new Master(*synth, &config);
        }

        void tearDown() {
            for(int i = 0; i < 16; ++i)
                delete master[i];

            delete[] outL;
            delete[] outR;
            delete synth;
        }


        void testInit() {

            for(int x=0; x<100; ++x)
                for(int i=0; i<16; ++i)
                    master[i]->GetAudioOutSamples(rand()%1025,
                            synth->samplerate, outL, outR);
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


        void testLoadSave(void)
        {
            const string fname = string(SOURCE_DIR) + "/guitar-adnote.xmz";
            const string fdata = loadfile(fname);
            char *result = NULL;
            master[0]->putalldata((char*)fdata.c_str());
            int res = master[0]->getalldata(&result);

            TS_ASSERT_EQUAL_INT((int)(fdata.length()+1), res);
            TS_ASSERT(fdata == result);
            if(fdata != result)
                print_string_differences(fdata, result);
        }


    private:
        float *outR, *outL;
        Master *master[16];
};

int main()
{
    PluginTest test;
    RUN_TEST(testInit);
    RUN_TEST(testPanic);
    RUN_TEST(testLoadSave);
}
