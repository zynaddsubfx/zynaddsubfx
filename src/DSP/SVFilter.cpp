/*
  ZynAddSubFX - a software synthesizer

  SVFilter.cpp - Several state-variable filters
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

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

#include <cmath>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <err.h>
#include "../Misc/Util.h"
#include "SVFilter.h"

SVFilter::SVFilter(unsigned char Ftype,
                   REALTYPE Ffreq,
                   REALTYPE Fq,
                   unsigned char Fstages)
{
    assert(Ftype<4);
    stages    = Fstages;
    type      = Ftype;
    freq      = Ffreq;
    q         = Fq;
    gain      = 1.0;
    outgain   = 1.0;
    needsinterpolation = false;
    firsttime = true;
    if(stages >= MAX_FILTER_STAGES)
        stages = MAX_FILTER_STAGES;
    cleanup();
    setfreq_and_q(Ffreq, Fq);
}

SVFilter::~SVFilter()
{}

void SVFilter::cleanup()
{
    for(int i = 0; i < MAX_FILTER_STAGES + 1; i++) {
        st[i].low   = 0.0;
        st[i].high  = 0.0;
        st[i].band  = 0.0;
        st[i].notch = 0.0;
    }
    oldabovenq = false;
    abovenq    = false;
}

void SVFilter::computefiltercoefs()
{
    par.f      = freq / SAMPLE_RATE * 4.0;
    if(par.f > 0.99999)
        par.f = 0.99999;
    par.q      = 1.0 - atan(sqrt(q)) * 2.0 / PI;
    par.q      = pow(par.q, 1.0 / (stages + 1));
    par.q_sqrt = sqrt(par.q);
}


void SVFilter::setfreq(REALTYPE frequency)
{
    if(frequency < 0.1)
        frequency = 0.1;
    REALTYPE rap = freq / frequency;
    if(rap < 1.0)
        rap = 1.0 / rap;

    oldabovenq = abovenq;
    abovenq    = frequency > (SAMPLE_RATE / 2 - 500.0);

    bool nyquistthresh = (abovenq ^ oldabovenq);

    //if the frequency is changed fast, it needs interpolation
    if((rap > 3.0) || nyquistthresh) {//(now, filter and coeficients backup)
        ipar = par;
        if(!firsttime)
            needsinterpolation = true;
    }
    freq      = frequency;
    computefiltercoefs();
    firsttime = false;
}

void SVFilter::setfreq_and_q(REALTYPE frequency, REALTYPE q_)
{
    q = q_;
    setfreq(frequency);
}

void SVFilter::setq(REALTYPE q_)
{
    q = q_;
    computefiltercoefs();
}

void SVFilter::settype(int type_)
{
    type = type_;
    computefiltercoefs();
}

void SVFilter::setgain(REALTYPE dBgain)
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

void SVFilter::singlefilterout(REALTYPE *smp, fstage &x, parameters &par)
{
    REALTYPE *out = NULL;
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
        errx(1, "Impossible SVFilter type encountered [%d]", type);
    }

    for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
        x.low   = x.low + par.f * x.band;
        x.high  = par.q_sqrt * smp[i] - x.low - par.q * x.band;
        x.band  = par.f * x.high + x.band;
        x.notch = x.high + x.low;

        smp[i]  = *out;
    }
}

void SVFilter::filterout(REALTYPE *smp)
{
    for(int i = 0; i < stages + 1; i++)
        singlefilterout(smp, st[i], par);

    if(needsinterpolation) {
        REALTYPE *ismp = getTmpBuffer();
        memcpy(ismp, smp, sizeof(REALTYPE) * SOUND_BUFFER_SIZE);

        for(int i = 0; i < stages + 1; i++)
            singlefilterout(ismp, st[i], ipar);

        for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
            REALTYPE x = i / (REALTYPE) SOUND_BUFFER_SIZE;
            smp[i] = ismp[i] * (1.0 - x) + smp[i] * x;
        }
        returnTmpBuffer(ismp);
        needsinterpolation = false;
    }

    for(int i = 0; i < SOUND_BUFFER_SIZE; i++)
        smp[i] *= outgain;
}

