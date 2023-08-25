/*
  ZynAddSubFX - a software synthesizer

  DynamicFilter.cpp - "WahWah" effect and others
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cassert>
#include <cmath>
#include <iostream>
#include "DynamicFilter.h"
#include "../DSP/Filter.h"
#include "../Misc/Allocator.h"
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>

namespace zyn {

#define rObject DynamicFilter
#define rBegin [](const char *msg, rtosc::RtData &d) {
#define rEnd }

rtosc::Ports DynamicFilter::ports = {
    {"preset::i", rOptions(WahWah, AutoWah, Sweep, VocalMorph1, VocalMorph2)
                  rDefault(0)
                  rDoc("Instrument Presets"), 0,
                  rBegin;
                  rObject *o = (rObject*)d.obj;
                  if(rtosc_narguments(msg))
                      o->setpreset(rtosc_argument(msg, 0).i);
                  else
                      d.reply(d.loc, "i", o->Ppreset);
                  rEnd},
    rPresetForVolume,
    rEffParVol(rDefaultDepends(presetOfVolume),
               rDefault(55), rPreset(2, 50), rPreset(4, 63),
               rPresetsAt(16, 110, 110, 100, 110, 127)),
    rEffParPan(),
    rEffPar(Pfreq,      2, rShort("freq"),   rPresets(80, 70, 30, 80, 50),
            "Effect Frequency"),
    rEffPar(Pfreqrnd,   3, rShort("rand"),   rDefault(0),
            "Frequency Randomness"),
    rEffParOpt(PLFOtype,   4, rShort("shape"),  rOptions(sin, tri),
            rDefault(sin), "LFO Shape"),
    rEffPar(PStereo,    5, rShort("stereo"), rPresets(64, 80, 50, 64, 96),
            "Stereo Mode"),
    rEffPar(Pdepth,     6, rShort("depth"),  rPresets(0, 70, 80, 0, 64),
            "LFO Depth"),
    rEffPar(Pampsns,    7, rShort("sense"),
            rPreset(0, 90) rPreset(3, 64) rDefault(0),
            "how the filter varies according to the input amplitude"),
    rEffPar(Pampsnsinv, 8, rShort("sns.inv"), rDefault(0),  "Sense Inversion"),
    rEffPar(Pampsmooth, 9, rShort("smooth"),  rDefault(60),
            "how smooth the input amplitude changes the filter"),
};
#undef rBegin
#undef rEnd
#undef rObject

DynamicFilter::DynamicFilter(EffectParams pars)
    :Effect(pars),
      lfo(pars.srate, pars.bufsize),
      Pvolume(110),
      Pdepth(0),
      Pampsns(90),
      Pampsnsinv(0),
      Pampsmooth(60),
      filterl(NULL),
      filterr(NULL)
{
    filterpars = pars.filterpars;
    setpreset(Ppreset, pars.filterprotect);
    cleanup();
}

DynamicFilter::~DynamicFilter()
{
    memory.dealloc(filterl);
    memory.dealloc(filterr);
}


// Apply the effect
void DynamicFilter::out(const Stereo<float *> &smp)
{
    if(filterpars->changed) {
        filterpars->changed = false;
        cleanup();
    }

    float lfol, lfor;
    lfo.effectlfoout(&lfol, &lfor);
    lfol *= depth * 5.0f;
    lfor *= depth * 5.0f;
    const float freq = filterpars->getfreq();
    const float q    = filterpars->getq();

    for(int i = 0; i < buffersize; ++i) {
        efxoutl[i] = smp.l[i];
        efxoutr[i] = smp.r[i];

        const float x = (fabsf(smp.l[i]) + fabsf(smp.r[i])) * 0.5f;
        ms1 = ms1 * (1.0f - ampsmooth) + x * ampsmooth + 1e-10;
    }

    const float ampsmooth2 = powf(ampsmooth, 0.2f) * 0.3f;
    ms2 = ms2 * (1.0f - ampsmooth2) + ms1 * ampsmooth2;
    ms3 = ms3 * (1.0f - ampsmooth2) + ms2 * ampsmooth2;
    ms4 = ms4 * (1.0f - ampsmooth2) + ms3 * ampsmooth2;
    const float rms = (sqrtf(ms4)) * ampsns;

    const float frl = Filter::getrealfreq(freq + lfol + rms);
    const float frr = Filter::getrealfreq(freq + lfor + rms);

    filterl->setfreq_and_q(frl, q);
    filterr->setfreq_and_q(frr, q);

    filterl->filterout(efxoutl);
    filterr->filterout(efxoutr);

    //panning
    for(int i = 0; i < buffersize; ++i) {
        efxoutl[i] *= pangainL;
        efxoutr[i] *= pangainR;
    }
}

// Cleanup the effect
void DynamicFilter::cleanup(void)
{
    reinitfilter();
    ms1 = ms2 = ms3 = ms4 = 0.0f;
}


//Parameter control
void DynamicFilter::setdepth(unsigned char _Pdepth)
{
    Pdepth = _Pdepth;
    depth  = powf(Pdepth / 127.0f, 2.0f);
}


void DynamicFilter::setvolume(unsigned char _Pvolume)
{
    Pvolume   = _Pvolume;
    outvolume = Pvolume / 127.0f;
    if(!insertion)
        volume = 1.0f;
    else
        volume = outvolume;
}

void DynamicFilter::setampsns(unsigned char _Pampsns)
{
    Pampsns = _Pampsns;
    ampsns  = powf(Pampsns / 127.0f, 2.5f) * 10.0f;
    if(Pampsnsinv)
        ampsns = -ampsns;
    ampsmooth = expf(-Pampsmooth / 127.0f * 10.0f) * 0.99f;
}

void DynamicFilter::reinitfilter(void)
{
    memory.dealloc(filterl);
    memory.dealloc(filterr);

    try {
        filterl = Filter::generate(memory, filterpars, samplerate, buffersize);
    } catch(std::bad_alloc& ba) {
        std::cerr << "failed to generate left filter for dynamic filter: " << ba.what() << std::endl;
    }

    try {
        filterr = Filter::generate(memory, filterpars, samplerate, buffersize);
    } catch(std::bad_alloc& ba) {
        std::cerr << "failed to generate right filter for dynamic filter: " << ba.what() << std::endl;
    }
}

void DynamicFilter::setfilterpreset(unsigned char npreset)
{
    filterpars->defaults();
    filterpars->updateLoc(dynfilter_0 + npreset);

    switch(npreset) {
        case 0:
            filterpars->Pcategory = 0;
            filterpars->Ptype     = 2;
            filterpars->basefreq  = FilterParams::basefreqFromOldPreq(45);
            filterpars->baseq     = FilterParams::baseqFromOldPq(64);
            filterpars->Pstages   = 1;
            filterpars->gain      = FilterParams::gainFromOldPgain(64);
            break;
        case 1:
            filterpars->Pcategory = 2;
            filterpars->Ptype     = 0;
            filterpars->basefreq  = FilterParams::basefreqFromOldPreq(72);
            filterpars->baseq     = FilterParams::baseqFromOldPq(64);
            filterpars->Pstages   = 0;
            filterpars->gain      = FilterParams::gainFromOldPgain(64);
            break;
        case 2:
            filterpars->Pcategory = 0;
            filterpars->Ptype     = 4;
            filterpars->basefreq  = FilterParams::basefreqFromOldPreq(64);
            filterpars->baseq     = FilterParams::baseqFromOldPq(64);
            filterpars->Pstages   = 2;
            filterpars->gain      = FilterParams::gainFromOldPgain(64);
            break;
        case 3:
            filterpars->Pcategory = 1;
            filterpars->Ptype     = 0;
            filterpars->basefreq  = FilterParams::basefreqFromOldPreq(50);
            filterpars->baseq     = FilterParams::baseqFromOldPq(70);
            filterpars->Pstages   = 1;
            filterpars->gain      = FilterParams::gainFromOldPgain(64);

            filterpars->Psequencesize = 2;
            // "I"
            filterpars->Pvowels[0].formants[0].freq = 34;
            filterpars->Pvowels[0].formants[0].amp  = 127;
            filterpars->Pvowels[0].formants[0].q    = 64;
            filterpars->Pvowels[0].formants[1].freq = 99;
            filterpars->Pvowels[0].formants[1].amp  = 122;
            filterpars->Pvowels[0].formants[1].q    = 64;
            filterpars->Pvowels[0].formants[2].freq = 108;
            filterpars->Pvowels[0].formants[2].amp  = 112;
            filterpars->Pvowels[0].formants[2].q    = 64;
            // "A"
            filterpars->Pvowels[1].formants[0].freq = 61;
            filterpars->Pvowels[1].formants[0].amp  = 127;
            filterpars->Pvowels[1].formants[0].q    = 64;
            filterpars->Pvowels[1].formants[1].freq = 71;
            filterpars->Pvowels[1].formants[1].amp  = 121;
            filterpars->Pvowels[1].formants[1].q    = 64;
            filterpars->Pvowels[1].formants[2].freq = 99;
            filterpars->Pvowels[1].formants[2].amp  = 117;
            filterpars->Pvowels[1].formants[2].q    = 64;
            break;
        case 4:
            filterpars->Pcategory = 1;
            filterpars->Ptype     = 0;
            filterpars->basefreq  = FilterParams::basefreqFromOldPreq(64);
            filterpars->baseq     = FilterParams::baseqFromOldPq(70);
            filterpars->Pstages   = 1;
            filterpars->gain      = FilterParams::gainFromOldPgain(64);

            filterpars->Psequencesize   = 2;
            filterpars->Pnumformants    = 2;
            filterpars->Pvowelclearness = 0;

            filterpars->Pvowels[0].formants[0].freq = 70;
            filterpars->Pvowels[0].formants[0].amp  = 127;
            filterpars->Pvowels[0].formants[0].q    = 64;
            filterpars->Pvowels[0].formants[1].freq = 80;
            filterpars->Pvowels[0].formants[1].amp  = 122;
            filterpars->Pvowels[0].formants[1].q    = 64;

            filterpars->Pvowels[1].formants[0].freq = 20;
            filterpars->Pvowels[1].formants[0].amp  = 127;
            filterpars->Pvowels[1].formants[0].q    = 64;
            filterpars->Pvowels[1].formants[1].freq = 100;
            filterpars->Pvowels[1].formants[1].amp  = 121;
            filterpars->Pvowels[1].formants[1].q    = 64;
            break;
    }

//	    for (int i=0;i<5;i++){
//		printf("freq=%d  amp=%d  q=%d\n",filterpars->Pvowels[0].formants[i].freq,filterpars->Pvowels[0].formants[i].amp,filterpars->Pvowels[0].formants[i].q);
//	    };
    reinitfilter();
}

unsigned char DynamicFilter::getpresetpar(unsigned char npreset, unsigned int npar)
{
#define	PRESET_SIZE 10
#define	NUM_PRESETS 5
    static const unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        //WahWah
        {110, 64, 80, 0, 0, 64, 0,  90, 0, 60},
        //AutoWah
        {110, 64, 70, 0, 0, 80, 70, 0,  0, 60},
        //Sweep
        {100, 64, 30, 0, 0, 50, 80, 0,  0, 60},
        //VocalMorph1
        {110, 64, 80, 0, 0, 64, 0,  64, 0, 60},
        //VocalMorph2
        {127, 64, 50, 0, 0, 96, 64, 0,  0, 60}
    };
    if(npreset < NUM_PRESETS && npar < PRESET_SIZE) {
        if(npar == 0 && insertion == 0) {
            /* lower the volume if this is system effect */
            return presets[npreset][npar] / 2;
        }
        return presets[npreset][npar];
    }
    return 0;
}

void DynamicFilter::setpreset(unsigned char npreset, bool protect)
{
    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n != 128; n++)
        changepar(n, getpresetpar(npreset, n));
    Ppreset = npreset;
    if(!protect)
        setfilterpreset(npreset);
}

void DynamicFilter::changepar(int npar, unsigned char value)
{
    switch(npar) {
        case 0:
            setvolume(value);
            break;
        case 1:
            setpanning(value);
            break;
        case 2:
            lfo.Pfreq = value;
            lfo.updateparams();
            break;
        case 3:
            lfo.Prandomness = value;
            lfo.updateparams();
            break;
        case 4:
            lfo.PLFOtype = value;
            lfo.updateparams();
            break;
        case 5:
            lfo.Pstereo = value;
            lfo.updateparams();
            break;
        case 6:
            setdepth(value);
            break;
        case 7:
            setampsns(value);
            break;
        case 8:
            Pampsnsinv = value;
            setampsns(Pampsns);
            break;
        case 9:
            Pampsmooth = value;
            setampsns(Pampsns);
            break;
    }
}

unsigned char DynamicFilter::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return lfo.Pfreq;
        case 3:  return lfo.Prandomness;
        case 4:  return lfo.PLFOtype;
        case 5:  return lfo.Pstereo;
        case 6:  return Pdepth;
        case 7:  return Pampsns;
        case 8:  return Pampsnsinv;
        case 9:  return Pampsmooth;
        default: return 0;
    }
}

}
