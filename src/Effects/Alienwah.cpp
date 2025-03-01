/*
  ZynAddSubFX - a software synthesizer

  Alienwah.cpp - "AlienWah" effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include <rtosc/port-sugar.h>
#include <rtosc/ports.h>
#include "../Misc/Allocator.h"
#include "Alienwah.h"

namespace zyn {

using std::complex;

#define rObject Alienwah
#define rBegin [](const char *msg, rtosc::RtData &d) {
#define rEnd }

rtosc::Ports Alienwah::ports = {
    {"preset::i", rProp(parameter)
                  rOptions(wah 1, wah 2, wah 3, wah 4)
                  rProp(alias)
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
               rDefault(63), rPresetsAt(3, 46),
               rPresetsAt(16, 127, 127, 127, 93)),
    rEffParPan(),
    rEffPar(Pfreq,     2, rShort("freq") rPresets(70, 73, 63, 25),
            "Effect Frequency"),
    rEffPar(Pfreqrnd,  3, rShort("rand"), rPreset(1, 106) rDefault(0),
            "Frequency Randomness"),
    rEffParOpt(PLFOtype,  4, rShort("shape"),
            rOptions(sine, triangle), rPresets(sine, sine, triangle, triangle),
            "LFO Shape"),
    rEffPar(PStereo,   5, rShort("stereo"), rPresets(62, 101, 100, 66),
            "Stereo Mode"),
    rEffPar(Pdepth,    6, rShort("depth"), rPresets(60, 60, 112, 101),
            "LFO Depth"),
    rEffPar(Pfeedback, 7, rShort("fb"), rPreset(3, 11), rDefault(105),
            "Feedback"),
    rEffPar(Pdelay,    8, rLinear(1,100), rPresets(25, 17, 31, 47),
            rShort("delay"), "Delay"),
    rEffPar(Plrcross,  9, rShort("l/r"), rDefault(0), "Left/Right Crossover"),
    rEffPar(Pphase,   10, rShort("phase"), rDefault(64), rPreset(2, 42),
            rPreset(3, 86), "Phase"),
};
#undef rBegin
#undef rEnd
#undef rObject

Alienwah::Alienwah(EffectParams pars)
    :Effect(pars),
      lfo(pars.srate, pars.bufsize),
      oldl(NULL),
      oldr(NULL)
{
    setpreset(Ppreset);
    cleanup();
    oldclfol = complex<float>(fb, 0.0f);
    oldclfor = complex<float>(fb, 0.0f);
}

Alienwah::~Alienwah()
{
    memory.devalloc(oldl);
    memory.devalloc(oldr);
}


//Apply the effect
void Alienwah::out(const Stereo<float *> &smp)
{
    float lfol, lfor; //Left/Right LFOs
    complex<float> clfol, clfor;
    /**\todo Rework, as optimization can be used when the new complex type is
     * utilized.
     * Before all calculations needed to be done with individual float,
     * but now they can be done together*/
    lfo.effectlfoout(&lfol, &lfor);
    lfol *= depth * PI * 2.0f;
    lfor *= depth * PI * 2.0f;
    clfol = complex<float>(cosf(lfol + phase) * fb, sinf(lfol + phase) * fb); //rework
    clfor = complex<float>(cosf(lfor + phase) * fb, sinf(lfor + phase) * fb); //rework

    for(int i = 0; i < buffersize; ++i) {
        float x  = ((float) i) / buffersize_f;
        float x1 = 1.0f - x;
        //left
        complex<float> tmp = clfol * x + oldclfol * x1;

        complex<float> out = tmp * oldl[oldk];
        out += (1 - fabsf(fb)) * smp.l[i] * pangainL;

        oldl[oldk] = out;
        float l = out.real() * 10.0f * (fb + 0.1f);

        //right
        tmp = clfor * x + oldclfor * x1;

        out = tmp * oldr[oldk];
        out += (1 - fabsf(fb)) * smp.r[i] * pangainR;

        oldr[oldk] = out;
        float r = out.real() * 10.0f * (fb + 0.1f);


        if(++oldk >= Pdelay)
            oldk = 0;
        //LRcross
        efxoutl[i] = l * (1.0f - lrcross) + r * lrcross;
        efxoutr[i] = r * (1.0f - lrcross) + l * lrcross;
    }

    oldclfol = clfol;
    oldclfor = clfor;
}

//Cleanup the effect
void Alienwah::cleanup(void)
{
    for(int i = 0; i < Pdelay; ++i) {
        oldl[i] = complex<float>(0.0f, 0.0f);
        oldr[i] = complex<float>(0.0f, 0.0f);
    }
    oldk = 0;
}


//Parameter control
void Alienwah::setdepth(unsigned char _Pdepth)
{
    Pdepth = _Pdepth;
    depth  = Pdepth / 127.0f;
}

void Alienwah::setfb(unsigned char _Pfb)
{
    Pfb = _Pfb;
    fb  = fabsf((Pfb - 64.0f) / 64.1f);
    fb  = sqrtf(fb);
    if(fb < 0.4f)
        fb = 0.4f;
    if(Pfb < 64)
        fb = -fb;
}

void Alienwah::setvolume(unsigned char _Pvolume)
{
    Pvolume   = _Pvolume;
    outvolume = Pvolume / 127.0f;
    if(insertion == 0)
        volume = 1.0f;
    else
        volume = outvolume;
}

void Alienwah::setphase(unsigned char _Pphase)
{
    Pphase = _Pphase;
    phase  = (Pphase - 64.0f) / 64.0f * PI;
}

void Alienwah::setdelay(unsigned char _Pdelay)
{
    memory.devalloc(oldl);
    memory.devalloc(oldr);
    Pdelay = limit<int>(_Pdelay, 1, MAX_ALIENWAH_DELAY);
    oldl   = memory.valloc<complex<float>>(Pdelay);
    oldr   = memory.valloc<complex<float>>(Pdelay);
    cleanup();
}

unsigned char Alienwah::getpresetpar(unsigned char npreset, unsigned int npar)
{
#define	PRESET_SIZE 11
#define	NUM_PRESETS 4
    static const unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        //AlienWah1
        {127, 64, 70, 0,   0, 62,  60,  105, 25, 0, 64},
        //AlienWah2
        {127, 64, 73, 106, 0, 101, 60,  105, 17, 0, 64},
        //AlienWah3
        {127, 64, 63, 0,   1, 100, 112, 105, 31, 0, 42},
        //AlienWah4
        {93,  64, 25, 0,   1, 66,  101, 11,  47, 0, 86}
    };
    if(npreset < NUM_PRESETS && npar < PRESET_SIZE) {
        if (npar == 0 && insertion == 0) {
            /* lower the volume if this is system effect */
            return presets[npreset][npar] / 2;
        }
        return presets[npreset][npar];
    }
    return 0;
}

void Alienwah::setpreset(unsigned char npreset)
{
    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n != 128; n++)
        changepar(n, getpresetpar(npreset, n));
    Ppreset = npreset;
}

void Alienwah::changepar(int npar, unsigned char value)
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
            setfb(value);
            break;
        case 8:
            setdelay(value);
            break;
        case 9:
            setlrcross(value);
            break;
        case 10:
            setphase(value);
            break;
    }
}

unsigned char Alienwah::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return lfo.Pfreq;
        case 3:  return lfo.Prandomness;
        case 4:  return lfo.PLFOtype;
        case 5:  return lfo.Pstereo;
        case 6:  return Pdepth;
        case 7:  return Pfb;
        case 8:  return Pdelay;
        case 9:  return Plrcross;
        case 10: return Pphase;
        default: return 0;
    }
}

}
