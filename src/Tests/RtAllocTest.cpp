/*
  ZynAddSubFX - a software synthesizer

  RtAllocTest.h - CxxTest for RT Safe Note Allocation
  Copyright (C) 2014 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/


#include <cxxtest/TestSuite.h>
#include <cstdio>
#include <malloc.h>
#include "../Misc/Part.h"
#include "../Misc/Util.h"
#include "../Misc/Allocator.h"
#include "../Params/Presets.h"
#include "../DSP/FFTwrapper.h"
#include "../globals.h"
SYNTH_T *synth;

using namespace std;

size_t allocations;
size_t allocated_size;
void *(*old_malloc_hook)(size_t, const void*);
void *hook(size_t size, const void *v)
{
    printf("Alloc(%d,%p)\n", size, v);
    allocated_size += size;
    allocations++;
    __malloc_hook = 0;
    void *res = malloc(size);
    __malloc_hook = hook;
    return res;
}


class RtAllocTest:public CxxTest::TestSuite
{
    public:

        Part *part;
        Microtonal   *micro;
        FFTwrapper   *fft;
        Allocator     memory;
        float *outR, *outL;

        void setUp() {
            //First the sensible settings and variables that have to be set:
            synth = new SYNTH_T;
            synth->buffersize = 256;

            outL = NULL;
            outR = NULL;
            denormalkillbuf = new float[synth->buffersize];

            //phew, glad to get thouse out of my way. took me a lot of sweat and gdb to get this far...
            fft = new FFTwrapper(synth->oscilsize);
            micro = new Microtonal();
            part = new Part(memory, micro, fft);
            part->partoutl = new float[synth->buffersize];
            part->partoutr = new float[synth->buffersize];
            //prepare the default settings
        }

        void tearDown() {
            delete part;
            delete micro;
            delete fft;
            FFT_cleanup();
            delete synth;
        }

        void testDefaults() {
            old_malloc_hook = __malloc_hook;
            __malloc_hook = hook;
            part->NoteOn(82, 101, 0);
            printf("allocated size = %u\n", allocated_size);
            printf("allocations    = %u\n", allocations);
        }
};
