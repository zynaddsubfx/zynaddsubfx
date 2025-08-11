/*
  ZynAddSubFX - a software synthesizer

  FormantFilter.cpp - formant filters
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include <cstdio>
#include "../Misc/Util.h"
#include "../Misc/Allocator.h"
#include "FormantFilter.h"
#include "AnalogFilter.h"
#include "../Params/FilterParams.h"

namespace zyn {

FormantFilter::FormantFilter(const FilterParams *pars, Allocator *alloc, unsigned int srate, int bufsize)
    :Filter(srate, bufsize), memory(*alloc)
{
    numformants = pars->Pnumformants;
    for(int i = 0; i < numformants; ++i)
        formant[i] = memory.alloc<AnalogFilter>(4 /*BPF*/, 1000.0f, 10.0f, pars->Pstages, srate, bufsize);
    cleanup();

    for(int j = 0; j < FF_MAX_VOWELS; ++j)
        for(int i = 0; i < numformants; ++i) {
            formantpar[j][i].freq = pars->getformantfreq(
                pars->Pvowels[j].formants[i].freq);
            formantpar[j][i].amp = pars->getformantamp(
                pars->Pvowels[j].formants[i].amp);
            formantpar[j][i].q = pars->getformantq(
                pars->Pvowels[j].formants[i].q);
        }

    for(int i = 0; i < FF_MAX_FORMANTS; ++i)
    {
        formant_amp_smoothing[i].sample_rate(srate);
        formant_amp_smoothing[i].reset(1.0f);
    }

    for(int i = 0; i < numformants; ++i) {
        currentformants[i].freq = 1000.0f;
        currentformants[i].amp  = 1.0f;
        currentformants[i].q    = 2.0f;
    }

    formantslowness = powf(1.0f - (pars->Pformantslowness / 128.0f), 3.0f);

    sequencesize = pars->Psequencesize;
    if(sequencesize == 0)
        sequencesize = 1;
    for(int k = 0; k < sequencesize; ++k)
        sequence[k].nvowel = pars->Psequence[k].nvowel;

    vowelclearness = powf(10.0f, (pars->Pvowelclearness - 32.0f) / 48.0f);

    sequencestretch = powf(0.1f, (pars->Psequencestretch - 32.0f) / 48.0f);
    if(pars->Psequencereversed)
        sequencestretch *= -1.0f;

    outgain = dB2rap(pars->getgain());

    oldinput   = -1.0f;
    Qfactor = pars->getq();
    oldQfactor = Qfactor;
    firsttime  = true;
}

FormantFilter::~FormantFilter()
{
    for(int i = 0; i < numformants; ++i)
        memory.dealloc(formant[i]);
}

void FormantFilter::cleanup()
{
    for(int i = 0; i < numformants; ++i)
        formant[i]->cleanup();
}

inline float log_2(float x)
{
    return logf(x) / logf(2.0f);
}

void FormantFilter::setpos(float frequency)
{
    int p1, p2;

    //Convert form real freq[Hz]
    const float input = log_2(frequency) - 9.96578428f; //log2(1000)=9.95748f.

    if(firsttime)
        slowinput = input;
    else
        slowinput = slowinput
                    * (1.0f - formantslowness) + input * formantslowness;

    if((fabsf(oldinput - input) < 0.001f) && (fabsf(slowinput - input) < 0.001f)
       && (fabsf(Qfactor - oldQfactor) < 0.001f)) {
        //      oldinput=input; setting this will cause problems at very slow changes
        firsttime = false;
        return;
    }
    else
        oldinput = input;

    float pos = input * sequencestretch;
    pos -= floorf(pos);

    F2I(pos * sequencesize, p2);
    p1 = p2 - 1;
    if(p1 < 0)
        p1 += sequencesize;

    pos = pos * sequencesize;
    pos -= floorf(pos);
    pos =
        (atanf((pos * 2.0f
                - 1.0f)
               * vowelclearness) / atanf(vowelclearness) + 1.0f) * 0.5f;

    p1 = sequence[p1].nvowel;
    p2 = sequence[p2].nvowel;

    if(firsttime) {
        for(int i = 0; i < numformants; ++i) {
            currentformants[i].freq =
                formantpar[p1][i].freq
                * (1.0f - pos) + formantpar[p2][i].freq * pos;
            currentformants[i].amp =
                formantpar[p1][i].amp
                * (1.0f - pos) + formantpar[p2][i].amp * pos;
            currentformants[i].q =
                formantpar[p1][i].q * (1.0f - pos) + formantpar[p2][i].q * pos;
            formant[i]->setfreq_and_q(currentformants[i].freq,
                                      currentformants[i].q * Qfactor);
        }
        firsttime = false;
    }
    else
        for(int i = 0; i < numformants; ++i) {
            currentformants[i].freq =
                currentformants[i].freq * (1.0f - formantslowness)
                + (formantpar[p1][i].freq
                   * (1.0f - pos) + formantpar[p2][i].freq * pos)
                * formantslowness;

            currentformants[i].amp =
                currentformants[i].amp * (1.0f - formantslowness)
                + (formantpar[p1][i].amp * (1.0f - pos)
                   + formantpar[p2][i].amp * pos) * formantslowness;

            currentformants[i].q = currentformants[i].q
                                   * (1.0f - formantslowness)
                                   + (formantpar[p1][i].q * (1.0f - pos)
                                      + formantpar[p2][i].q
                                      * pos) * formantslowness;


            formant[i]->setfreq_and_q(currentformants[i].freq,
                                      currentformants[i].q * Qfactor);
        }

    oldQfactor = Qfactor;
}

void FormantFilter::setfreq(float frequency)
{
    setpos(frequency);
}

void FormantFilter::setq(float q_)
{
    Qfactor = q_;
    for(int i = 0; i < numformants; ++i)
        formant[i]->setq(Qfactor * currentformants[i].q);
}

void FormantFilter::setgain(float /*dBgain*/)
{}

void FormantFilter::setfreq_and_q(float frequency, float q_)
{
    Qfactor = q_;
    setpos(frequency);
}


void FormantFilter::filterout(float *smp)
{
    memcpy(filteroutInbuffer.data(), smp, bufferbytes);
    memset(smp, 0, bufferbytes);

    for(int j = 0; j < numformants; ++j) {

        for(int i = 0; i < buffersize; ++i)
            filteroutTmpbuf[i] = filteroutInbuffer[i] * outgain;

        formant[j]->filterout(filteroutTmpbuf.data());

        if ( formant_amp_smoothing[j].apply( filteroutFormantbuf.data(), buffersize, currentformants[j].amp ) )
        {
            for(int i = 0; i < buffersize; ++i)
                smp[i] += filteroutTmpbuf[i] * filteroutFormantbuf[i];
        }
        else
        {
            for(int i = 0; i < buffersize; ++i)
                smp[i] += filteroutTmpbuf[i] * currentformants[j].amp;
        }
    }
}

}
