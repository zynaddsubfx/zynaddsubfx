/*
  ZynAddSubFX - a software synthesizer

  Sympathetic.cpp - Distorsion effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "Sympathetic.h"
#include "../DSP/AnalogFilter.h"
#include "CombFilterBank.h"
#include "../Misc/Allocator.h"
#include <cmath>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include "../globals.h"

namespace zyn {

#define rObject Sympathetic
#define rBegin [](const char *msg, rtosc::RtData &d) {
#define rEnd }

rtosc::Ports Sympathetic::ports = {
    rEffParVol(rDefault(127)),
    rEffParPan(),
    rEffPar(Pq, 2, rShort("q"), rDefault(65), 
            "Resonance"),
    rEffPar(Pdrive,   3, rShort("drive"), rDefault(65),
            "Input Amplification"),
    rEffPar(Plevel,   4, rShort("output"), rDefault(65),
            "Output Amplification"),
    rEffPar(Punison_frequency_spread,  5, rShort("detune"), rDefault(30),
            "Unison String Detune"),
    rEffParTF(Pnegate, 6, rShort("neg"), rDefault(false), "Negate Signal"),
    rEffPar(Plpf, 7, rShort("lpf"), rDefault(127), "Low Pass Cutoff"),
    rEffPar(Phpf, 8, rShort("hpf"), rDefault(0), "High Pass Cutoff"),
    rEffPar(Punison_size, 9, rShort("unison"), rDefault(1),
            "Number of Unison Strings"),
};

#undef rBegin
#undef rEnd
#undef rObject

Sympathetic::Sympathetic(EffectParams pars)
    :Effect(pars),
      Pvolume(127),
      Pdrive(65),
      Plevel(65),
      Ptype(0),
      Pnegate(0),
      Plpf(127),
      Phpf(0),
      Pstereo(0),
      Pq(65),
      Punison_size(1),
      Punison_frequency_spread(30)
{
    lpfl = memory.alloc<AnalogFilter>(2, 22000, 1, 0, pars.srate, pars.bufsize);
    lpfr = memory.alloc<AnalogFilter>(2, 22000, 1, 0, pars.srate, pars.bufsize);
    hpfl = memory.alloc<AnalogFilter>(3, 20, 1, 0, pars.srate, pars.bufsize);
    hpfr = memory.alloc<AnalogFilter>(3, 20, 1, 0, pars.srate, pars.bufsize);
    
    filterBank = memory.alloc<CombFilterBank>(&memory, pars.srate, pars.bufsize, NUM_SYMPATHETIC_STRINGS);
    calcFreqs(Punison_size,Punison_frequency_spread); // sets freqs
    setpreset(Ppreset);
    cleanup();
}

Sympathetic::~Sympathetic()
{
    memory.dealloc(lpfl);
    memory.dealloc(lpfr);
    memory.dealloc(hpfl);
    memory.dealloc(hpfr);
    memory.dealloc(filterBank);
}

//Cleanup the effect
void Sympathetic::cleanup(void)
{
    lpfl->cleanup();
    hpfl->cleanup();
    lpfr->cleanup();
    hpfr->cleanup();
}


//Apply the filters
void Sympathetic::applyfilters(float *efxoutl, float *efxoutr)
{
    if(Plpf!=127) lpfl->filterout(efxoutl);
    if(Phpf!=0) hpfl->filterout(efxoutl);
    if(Pstereo != 0) { //stereo
        if(Plpf!=127) lpfr->filterout(efxoutr);
        if(Phpf!=0) hpfr->filterout(efxoutr);
    }
}


//Effect output
void Sympathetic::out(const Stereo<float *> &smp)
{
    float inputvol = powf(5.0f, (Pdrive - 32.0f) / 127.0f);
    if(Pnegate)
        inputvol *= -1.0f;

    if(Pstereo) //Stereo
        for(int i = 0; i < buffersize; ++i) {
            efxoutl[i] = smp.l[i] * inputvol * pangainL;
            efxoutr[i] = smp.r[i] * inputvol * pangainR;
        }
    else //Mono
        for(int i = 0; i < buffersize; ++i)
            efxoutl[i] = (smp.l[i] * pangainL + smp.r[i] * pangainR) * inputvol;

    filterBank->filterout(efxoutl);
    if(Pstereo)
        filterBank->filterout(efxoutr);

    applyfilters(efxoutl, efxoutr);

    if(!Pstereo)
        memcpy(efxoutr, efxoutl, bufferbytes);

    float level = dB2rap(60.0f * Plevel / 127.0f - 40.0f);
    for(int i = 0; i < buffersize; ++i) {
        float lout = efxoutl[i];
        float rout = efxoutr[i];
        float l    = lout * (1.0f - lrcross) + rout * lrcross;
        float r    = rout * (1.0f - lrcross) + lout * lrcross;
        lout = l;
        rout = r;

        efxoutl[i] = lout * 2.0f * level;
        efxoutr[i] = rout * 2.0f * level;
    }
}

//Parameter control
void Sympathetic::setvolume(unsigned char _Pvolume)
{
    Pvolume = _Pvolume;

    if(insertion == 0) {
        outvolume = powf(0.01f, (1.0f - Pvolume / 127.0f)) * 4.0f;
        volume    = 1.0f;
    }
    else
        volume = outvolume = Pvolume / 127.0f;
    if(Pvolume == 0)
        cleanup();
}

void Sympathetic::setlpf(unsigned char _Plpf)
{
    Plpf = _Plpf;
    float fr = expf(sqrtf(Plpf / 127.0f) * logf(25000.0f)) + 40.0f;
    lpfl->setfreq(fr);
    lpfr->setfreq(fr);
}

void Sympathetic::sethpf(unsigned char _Phpf)
{
    Phpf = _Phpf;
    float fr = expf(sqrtf(Phpf / 127.0f) * logf(25000.0f)) + 20.0f;
    hpfl->setfreq(fr);
    hpfr->setfreq(fr);
}

void Sympathetic::calcFreqs(unsigned char unison_size, unsigned char spread)
{

    if(spread!=spread_old)
    {
        spread_old = spread;
        float unison_spread_semicent = powf(spread / 63.5f, 2.0f) * 25.0f;
        unison_real_spread_up = powf(2.0f, (unison_spread_semicent * 0.5f) / 1200.0f);
        unison_real_spread_down = 1.0f/unison_real_spread_up;
    }

    filterBank-> nrOfStrings = 12;
    for(unsigned int i = 0; i < 12; ++i)
    {
        filterBank->freqs[i] = powf(2.0f, (float)i / 12.0f) * 220.0f;
    }
    
    if (unison_size > 1) 
    {
        filterBank->nrOfStrings = 24;
        for(unsigned int i = 12; i < 24; ++i)
        {
            filterBank->freqs[i] = powf(2.0f, (float)(i-12) / 12.0f) * 440.0f * unison_real_spread_up;
        }
    }
    
    if (unison_size > 2)
    {
        filterBank->nrOfStrings = 36;
        for(unsigned int i = 24; i < 36; ++i)
        {
            filterBank->freqs[i] = powf(2.0f, (float)(i-24) / 12.0f) * 440.0f * unison_real_spread_down;
        }
    }
    

}

unsigned char Sympathetic::getpresetpar(unsigned char npreset, unsigned int npar)
{
#define	PRESET_SIZE 13
#define	NUM_PRESETS 6
    static const unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        //Overdrive 1
        {127, 64, 35, 56, 70, 0, 0, 96,  0,   0, 0, 32, 64},
        //Overdrive 2
        {127, 64, 35, 29, 75, 1, 0, 127, 0,   0, 0, 32, 64},
        //A. Exciter 1
        {64,  64, 35, 75, 80, 5, 0, 127, 105, 1, 0, 32, 64},
        //A. Exciter 2
        {64,  64, 35, 85, 62, 1, 0, 127, 118, 1, 0, 32, 64},
        //Guitar Amp
        {127, 64, 35, 63, 75, 2, 0, 55,  0,   0, 0, 32, 64},
        //Quantisize
        {127, 64, 35, 88, 75, 4, 0, 127, 0,   1, 0, 32, 64}
    };
    if(npreset < NUM_PRESETS && npar < PRESET_SIZE) {
        if(npar == 0 && insertion == 0) {
            /* lower the volume if this is system effect */
            return (3 * presets[npreset][npar]) / 2;
        }
        return presets[npreset][npar];
    }
    return 0;
}

void Sympathetic::setpreset(unsigned char npreset)
{
    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n != 128; n++)
        changepar(n, getpresetpar(npreset, n));
    Ppreset = npreset;
    cleanup();
}


void Sympathetic::changepar(int npar, unsigned char value)
{
    switch(npar) {
        case 0:
            setvolume(value);
            break;
        case 1:
            setpanning(value);
            break;
        case 2:
            Pq = value;
            filterBank->gainbwd = 0.873f + (float)value/1000.0f;
            break;
        case 3:
            Pdrive = value;
            filterBank->inputgain = (float)Pdrive/65.0f;
            break;
        case 4:
            Plevel = value;
            filterBank->outgain = (float)Plevel/65.0f;
            break;
        case 5:
            Punison_frequency_spread = value;
            calcFreqs(Punison_size, Punison_frequency_spread);
            break;
        case 6:
            if(value > 1)
                Pnegate = 1;
            else
                Pnegate = value;
            break;
        case 7:
            setlpf(value);
            break;
        case 8:
            sethpf(value);
            break;
        case 9:
            Punison_size = limit(value, (unsigned char) 1, (unsigned char) 3);
            calcFreqs(Punison_size, Punison_frequency_spread);
            break;
    }
}

unsigned char Sympathetic::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return Pq;
        case 3:  return Pdrive;
        case 4:  return Plevel;
        case 5:  return Punison_frequency_spread;
        case 6:  return Pnegate;
        case 7:  return Plpf;
        case 8:  return Phpf;
        case 9:  return Punison_size;
        default: return 0; //in case of bogus parameter number
    }
}

}
