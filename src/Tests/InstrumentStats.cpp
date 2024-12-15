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
#include "../Misc/Sync.h"
#include "../Misc/MiddleWare.h"
#include "../Misc/Part.h"
#include "../Misc/Util.h"
#include "../Misc/Allocator.h"
#include "../Misc/Microtonal.h"
#include "../DSP/FFTwrapper.h"
#include "../globals.h"

#include "../Effects/EffectMgr.h"
#include "../Params/LFOParams.h"
#include "../Params/EnvelopeParams.h"
#include "../Params/ADnoteParameters.h"
#include "../Params/PADnoteParameters.h"
#include "../Params/SUBnoteParameters.h"
using namespace std;
using namespace zyn;

SYNTH_T *synth;
AbsTime *time_;
Sync *sync_;

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
    sync_ = new Sync();
    //for those patches that are just really big
    alloc.addMemory(malloc(1024*1024),1024*1024);

    outL  = new float[1024];
    for(int i = 0; i < synth->buffersize; ++i)
        outL[i] = 0.0f;
    outR = new float[1024];
    for(int i = 0; i < synth->buffersize; ++i)
        outR[i] = 0.0f;

    p = new Part(alloc, *synth, *time_, sync_, compress, interp, &microtonal, &fft);
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

/*
 *  Kit fields used
 *
 *  Kit type
 *
 *  Add synth engines used
 *    Add synth voices  used
 *  Sub synth engines used
 *  Pad synth engines used
 *
 *
 *  Total Envelopes
 *  Optional Envelopes
 *
 *  Total Free mode Envelopes
 *
 *  Total LFO
 *  Optional LFO
 *
 *  Total Filters
 *
 *  Total 'Analog' Filters
 *  Total SVF Filters
 *  Total Formant Filters
 *
 *  Total Effects
 */
void usage_stats(void)
{
    int kit_type        = 0;
    int kits_used       = 0;
    int add_engines     = 0;
    int add_voices      = 0;
    int sub_engines     = 0;
    int pad_engines     = 0;

    int env_total       = 0;
    int env_optional    = 0;
    int env_free        = 0;

    int lfo_total       = 0;
    int lfo_optional    = 0;

    int filter_total    = 0;
    int filter_analog   = 0;
    int filter_svf      = 0;
    int filter_formant  = 0;

    int effects_total   = 0;

    kit_type = p->Pkitmode;
    for(int i=0; i<NUM_KIT_ITEMS; ++i) {
        auto &k = p->kit[i];
        if(!(k.Penabled || (i==0 && p->Pkitmode == 0l)))
            continue;

        if(k.Padenabled) {
            auto &e = *k.adpars;
            add_engines += 1;
            for(int j=0; j<NUM_VOICES; ++j) {
                auto &v = k.adpars->VoicePar[j];
                if(!v.Enabled)
                    continue;
                add_voices += 1;
                if(v.PFilterEnabled) {
                    auto &f = *v.VoiceFilter;
                    filter_total += 1;
                    if(f.Pcategory == 0)
                        filter_analog += 1;
                    else if(f.Pcategory == 1)
                        filter_formant += 1;
                    else
                        filter_svf += 1;
                }
                if(v.PFreqLfoEnabled && v.FreqLfo->Pintensity) {
                    lfo_optional += 1;
                    lfo_total    += 1;
                }
                if(v.PFilterLfoEnabled && v.FilterLfo->Pintensity) {
                    lfo_optional += 1;
                    lfo_total    += 1;
                }
                if(v.PAmpLfoEnabled && v.AmpLfo->Pintensity) {
                    lfo_optional += 1;
                    lfo_total    += 1;
                }
                if(v.PFreqEnvelopeEnabled) {
                    env_optional += 1;
                    env_total    += 1;
                    env_free += !!v.FreqEnvelope->Pfreemode;
                }
                if(v.PFilterEnvelopeEnabled) {
                    env_optional += 1;
                    env_total    += 1;
                    env_free += !!v.FilterEnvelope->Pfreemode;
                }
                if(v.PAmpEnvelopeEnabled) {
                    env_optional += 1;
                    env_total    += 1;
                    env_free += !!v.AmpEnvelope->Pfreemode;
                }
            }



            if(e.GlobalPar.GlobalFilter) {
                auto &f = *e.GlobalPar.GlobalFilter;
                filter_total += 1;
                if(f.Pcategory == 0)
                    filter_analog += 1;
                else if(f.Pcategory == 1)
                    filter_formant += 1;
                else
                    filter_svf += 1;
            }
            if(e.GlobalPar.FreqLfo->Pintensity)
                lfo_total += 1;
            if(e.GlobalPar.FilterLfo->Pintensity)
                lfo_total += 1;
            if(e.GlobalPar.AmpLfo->Pintensity)
                lfo_total += 1;
            env_total += 3;
            env_free += !!e.GlobalPar.FreqEnvelope->Pfreemode;
            env_free += !!e.GlobalPar.FilterEnvelope->Pfreemode;
            env_free += !!e.GlobalPar.AmpEnvelope->Pfreemode;
        }

        if(k.Ppadenabled) {
            pad_engines += 1;
            auto &e = *k.padpars;
            if(e.GlobalFilter) {
                auto &f = *e.GlobalFilter;
                filter_total += 1;
                if(f.Pcategory == 0)
                    filter_analog += 1;
                else if(f.Pcategory == 1)
                    filter_formant += 1;
                else
                    filter_svf += 1;
            }
            if(e.FreqLfo->Pintensity)
                lfo_total += 1;
            if(e.FilterLfo->Pintensity)
                lfo_total += 1;
            if(e.AmpLfo->Pintensity)
                lfo_total += 1;
            env_total += 3;
            env_free += !!e.FreqEnvelope->Pfreemode;
            env_free += !!e.FilterEnvelope->Pfreemode;
            env_free += !!e.AmpEnvelope->Pfreemode;
        }

        if(k.Psubenabled) {
            sub_engines += 1;
            auto &e = *k.subpars;

            if(e.PGlobalFilterEnabled) {
                auto &f = *e.GlobalFilter;
                filter_total += 1;
                if(f.Pcategory == 0)
                    filter_analog += 1;
                else if(f.Pcategory == 1)
                    filter_formant += 1;
                else
                    filter_svf += 1;
            }
            if(e.PFreqEnvelopeEnabled) {
                env_total += 1;
                env_optional += 1;
                env_free  += !!e.FreqEnvelope->Pfreemode;
            }
            if(e.PGlobalFilterEnabled) {
                env_total += 1;
                env_optional += 1;
                env_free  += !!e.GlobalFilterEnvelope->Pfreemode;
            }
            if(e.PBandWidthEnvelopeEnabled) {
                env_total += 1;
                env_optional += 1;
                env_free  += !!e.BandWidthEnvelope->Pfreemode;
            }
        }

        kits_used += 1;
    }

    for(int i=0; i<NUM_PART_EFX; ++i) {
        if(p->partefx[i]->efx)
            effects_total += 1;
    }

    printf("Kit type:       %d\n", kit_type);
    printf("Kits used:      %d\n", kits_used);
    printf("Add engines:    %d\n", add_engines);
    printf("    Add voices:    %d\n", add_voices);
    printf("Sub engines:    %d\n", sub_engines);
    printf("Pad engines:    %d\n", pad_engines);

    printf("\n");

    printf("Env total:      %d\n", env_total);
    printf("Env optional:   %d\n", env_optional);
    printf("Env free:       %d\n", env_free);

    printf("\n");

    printf("LFO total:      %d\n", lfo_total);
    printf("LFO optional:   %d\n", lfo_optional);

    printf("\n");

    printf("Filter total:   %d\n", filter_total);
    printf("Filter analog:  %d\n", filter_analog);
    printf("Filter svf:     %d\n", filter_svf);
    printf("Filter formant: %d\n", filter_formant);

    printf("\n");

    printf("Effects Total:  %d\n", effects_total);
}

int main(int argc, char **argv)
{
    if(argc < 2) {
        fprintf(stderr, "Please supply a xiz file\n");
        return 1;
    }
    if(argc == 2) {
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
    } else if(argc == 3) {
        mode = MODE_TEST;
        setup();
        xml(argv[2]);
        load();
        usage_stats();
    }
}
