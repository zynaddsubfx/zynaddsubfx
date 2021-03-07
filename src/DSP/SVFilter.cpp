/*
  ZynAddSubFX - a software synthesizer

  SVFilter.cpp - Several state-variable filters
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include <cstdio>
#include <cstring>
#include <cassert>
#include "../Misc/Util.h"
#include "SVFilter.h"

#define errx(...) {}
#define warnx(...) {}
#ifndef errx
#include <err.h>
#endif

namespace zyn {

SVFilter::SVFilter(unsigned char Ftype, float Ffreq, float Fq,
                   unsigned char Fstages, unsigned int srate, int bufsize)
    :Filter(srate, bufsize),
      type(Ftype),
      stages(Fstages),
      freq(Ffreq),
      q(Fq),
      gain(1.0f)
{
    if(stages >= MAX_FILTER_STAGES)
        stages = MAX_FILTER_STAGES;
    outgain = 1.0f;
    cleanup();
    setfreq_and_q(Ffreq, Fq);
    freq_smoothing.reset(Ffreq);
    freq_smoothing.sample_rate(srate);
}

SVFilter::~SVFilter()
{}

void SVFilter::cleanup()
{
    for(int i = 0; i < MAX_FILTER_STAGES + 1; ++i)
        st[i].low = st[i].high = st[i].band = st[i].notch = 0.0f;
}

SVFilter::response::response(float b0, float b1, float b2,
                             float a0, float a1 ,float a2)
{
    a[0] = a0;
    a[1] = a1;
    a[2] = a2;
    b[0] = b0;
    b[1] = b1;
    b[2] = b2;
}

SVFilter::response SVFilter::computeResponse(int type,
        float freq, float pq, int stages, float gain, float fs)
{
    typedef SVFilter::response res;
    float f = freq / fs * 4.0;
    if(f > 0.99999f)
       f = 0.99999f;
    float q   = 1.0f - atanf(sqrtf(pq)) * 2.0f / PI;
    q         =  powf(q, 1.0f / (stages + 1));
    float qrt = sqrtf(q);
    float g   = powf(gain, 1.0 / (stages + 1));
    if(type == 0) { //Low
        return res{0, g*f*f*qrt, 0,
                   1,   (q*f+f*f-2),    (1-q*f)};
    }
    if(type == 1) {//High
        //g *= qrt/(1+f*q);
        g *= qrt;
        return res{g,    -2*g,    g,
                   //1,   (f*f-2*f*q-2)/(1+f*q),    1};
                   1,   (q*f+f*f-2),    (1-q*f)};
    }
    if(type == 2) {//Band
        g *= f*qrt;
        return res{g,   -g, 0,
                   1,   (q*f+f*f-2),    (1-q*f)};
    }
    if(type == 3 || true) {//Notch
        g *= qrt;
        return res{g, -2*g+g*f*f, g,
                   1,   (q*f+f*f-2),    (1-q*f)};
    }
}



void SVFilter::computefiltercoefs(void)
{
    par.f = freq / samplerate_f * 4.0f;
    if(par.f > 0.99999f)
        par.f = 0.99999f;
    par.q      = 1.0f - atanf(sqrtf(q)) * 2.0f / PI;
    par.q      = powf(par.q, 1.0f / (stages + 1));
    par.q_sqrt = sqrtf(par.q);
}


void SVFilter::setfreq(float frequency)
{
    if(frequency < 0.1f)
        frequency = 0.1f;
    float rap = freq / frequency;
    if(rap < 1.0f)
        rap = 1.0f / rap;

    freq = frequency;
    computefiltercoefs();
}

void SVFilter::setfreq_and_q(float frequency, float q_)
{
    q = q_;
    setfreq(frequency);
}

void SVFilter::setq(float q_)
{
    q = q_;
    computefiltercoefs();
}

void SVFilter::settype(int type_)
{
    type = type_;
    computefiltercoefs();
}

void SVFilter::setgain(float dBgain)
{
    gain = dB2rap(dBgain);
    computefiltercoefs();
}

void SVFilter::setstages(int stages_)
{
    if(stages_ >= MAX_FILTER_STAGES)
        stages_ = MAX_FILTER_STAGES - 1;
    stages = stages_;
    cleanup();
    computefiltercoefs();
}

float *SVFilter::getfilteroutfortype(SVFilter::fstage &x) {
    float *out = NULL;
    switch(type) {
        case 0:
            out = &x.low;
            break;
        case 1:
            out = &x.high;
            break;
        case 2:
            out = &x.band;
            break;
        case 3:
            out = &x.notch;
            break;
        default:
            out = &x.low;
            warnx("Impossible SVFilter type encountered [%d]", type);
    }
    return out;
}


// simplifying the responses
// xl = xl*z(-1) +      pf*xb*z(-1)
// xh = pq1*x    - xl - pq*xb*z(-1)
// xb = pf*xh    +         xb*z(-1)
// xn = xh       + xl
//
// xl = pf*xb*z(-1)/(1-z(-1))
// xb = pf*xh/(1-z(-1))
// xl = pf*pfxh*z(-1)/(1-z(-1))^2



void SVFilter::singlefilterout(float *smp, SVFilter::fstage &x, SVFilter::parameters &par, int buffersize )
{
    float *out = getfilteroutfortype(x);
    for(int i = 0; i < buffersize; ++i) {
        x.low   = x.low + par.f * x.band;
        x.high  = par.q_sqrt * smp[i] - x.low - par.q * x.band;
        x.band  = par.f * x.high + x.band;
        x.notch = x.high + x.low;
        smp[i]  = *out;
    }
}

void SVFilter::filterout(float *smp)
{
    assert((buffersize % 8) == 0);

    float freqbuf[buffersize];

    if ( freq_smoothing.apply( freqbuf, buffersize, freq ) )
    {
        /* 8 sample chunks seems to work OK for AnalogFilter, so do that here too. */
        for ( int i = 0; i < buffersize; i += 8 )
        {
            freq = freqbuf[i];
            computefiltercoefs();

            for(int j = 0; j < stages + 1; ++j)
                singlefilterout(smp + i, st[j], par, 8 );
        }

        freq = freqbuf[buffersize - 1];
        computefiltercoefs();
    }
    else
        for(int i = 0; i < stages + 1; ++i)
            singlefilterout(smp, st[i], par, buffersize );

    for(int i = 0; i < buffersize; ++i)
        smp[i] *= outgain;
}

}
