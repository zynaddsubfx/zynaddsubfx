/*
  ZynAddSubFX - a software synthesizer

  InstrumentStats.cpp - Instrument Profiler and Regression Tester
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include "../Misc/Time.h"
#include "../Misc/MiddleWare.h"
#include "../Misc/Part.h"
#include "../Misc/Util.h"
#include "../Misc/Allocator.h"
#include "../Misc/Microtonal.h"
#include "../DSP/FFTwrapper.h"
#include "../globals.h"
SYNTH_T *synth;
AbsTime *time_;

using namespace std;

char *instance_name=(char*)"";
MiddleWare *middleware;

enum RunMode {
    MODE_PROFILE,
    MODE_TEST,
};

RunMode mode;

int compress = 0;
float *outL, *outR;
Alloc      alloc;
Microtonal microtonal(compress);
FFTwrapper fft(1024);
Part *p;

double tic()
{
    timespec ts;
    // clock_gettime(CLOCK_MONOTONIC, &ts); // Works on FreeBSD
    clock_gettime(CLOCK_REALTIME, &ts);
    double t = ts.tv_sec;
    t += 1e-9*ts.tv_nsec;
    return t;
}

double toc()
{
    return tic();
}

int interp=1;
void setup() {
    synth = new SYNTH_T;
    synth->buffersize = 256;
    synth->samplerate = 48000;
    synth->alias();
    time_ = new AbsTime(*synth);
    //for those patches that are just really big
    alloc.addMemory(malloc(1024*1024),1024*1024);

    outL  = new float[1024];
    for(int i = 0; i < synth->buffersize; ++i)
        outL[i] = 0.0f;
    outR = new float[1024];
    for(int i = 0; i < synth->buffersize; ++i)
        outR[i] = 0.0f;

    p = new Part(alloc, *synth, *time_, compress, interp, &microtonal, &fft);
}

void xml(string s)
{
    double t_on = tic();
    p->loadXMLinstrument(s.c_str());
    double t_off = toc();
    if(mode == MODE_PROFILE)
        printf("%f, ", t_off - t_on);
}

void load()
{
    double t_on = tic();
    p->applyparameters();
    p->initialize_rt();
    double t_off = toc();
    if(mode == MODE_PROFILE)
        printf("%f, ", t_off - t_on);
}

void noteOn()
{
    int total_notes = 0;
    double t_on = tic(); // timer before calling func
    for(int i=40; i<100; ++i)
        total_notes += p->NoteOn(i,100,0);
    double t_off = toc(); // timer when func returns
    printf("%d, ", total_notes);
    if(mode == MODE_PROFILE)
        printf("%f, ", t_off - t_on);
}

void speed()
{
    const int samps = 150;

    double t_on = tic(); // timer before calling func
    for(int i = 0; i < samps; ++i) {
        p->ComputePartSmps();
        if(mode==MODE_TEST)
            printf("%f, %f, ", p->partoutl[0], p->partoutr[0]);
    }
    double t_off = toc(); // timer when func returns

    if(mode==MODE_PROFILE)
        printf("%f, %d, ", t_off - t_on, samps*synth->buffersize);
}

void noteOff()
{
    double t_on = tic(); // timer before calling func
    p->AllNotesOff();
    p->ComputePartSmps();
    double t_off = toc(); // timer when func returns
    if(mode == MODE_PROFILE)
        printf("%f, ", t_off - t_on);
}

void memUsage()
{
    if(mode == MODE_PROFILE)
        printf("%lld", alloc.totalAlloced());
}

int main(int argc, char **argv)
{
    if(argc != 2) {
        fprintf(stderr, "Please supply a xiz file\n");
        return 1;
    }

    mode = MODE_PROFILE;
    setup();
    xml(argv[1]);
    load();
    memUsage();
    printf(", ");
    noteOn();
    speed();
    noteOff();
    memUsage();
    printf("\n");
}
