/*
  ZynAddSubFX - a software synthesizer

  Chorus.cpp - Chorus and Flange effects
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include "../Misc/Allocator.h"
#include "Chorus.h"
#include <iostream>
using namespace std;

namespace zyn {

#define rObject Chorus
#define rBegin [](const char *msg, rtosc::RtData &d) {
#define rEnd }

rtosc::Ports Chorus::ports = {
    {"preset::i", rProp(parameter)
                  rOptions(Chorus1, Chorus2, Chorus3, Celeste1, Celeste2,
                           Flange1, Flange2, Flange3, Flange4, Flange5)
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
    //Pvolume/Ppanning are common
    rEffParVol(rDefault(64)),
    rEffParPan(),
    rEffPar(Pfreq,    2, rShort("freq"),
            rPresets(50, 45, 29, 26, 29, 57, 33, 53, 40, 55),
            "Effect Frequency"),
    rEffPar(Pfreqrnd, 3, rShort("rand"),
            rPreset(4, 117) rPreset(6, 34) rPreset(7, 34) rPreset(9, 105)
            rDefault(0), "Frequency Randomness"),
    rEffParOpt(PLFOtype, 4, rShort("shape"),
            rOptions(sine, tri),
            rPresets(sine, sine, tri, sine, sine, sine, tri, tri, tri, sine),
            "LFO Shape"),
    rEffPar(PStereo,  5, rShort("stereo"),
            rPresets(90, 98, 42, 42, 50, 60, 40, 94, 62, 24), "Stereo Mode"),
    rEffPar(Pdepth,   6, rShort("depth"),
            rPresets(40, 56, 97, 115, 115, 23, 35, 35, 12, 39), "LFO Depth"),
    rEffPar(Pdelay,   7, rShort("delay"),
            rPresets(85, 90, 95, 18, 9, 3, 3, 3, 19, 19), "Delay"),
    rEffPar(Pfeedback,8, rShort("fb"),
            rPresets(64, 64, 90, 90, 31, 62, 109, 54, 97, 17), "Feedback"),
    rEffPar(Plrcross, 9, rShort("l/r"), rPresets(119, 19, 127, 127, 127),
            rDefault(0), "Left/Right Crossover"),
    rEffParTF(Pflangemode, 10, rShort("flange"), rDefault(false),
              "Flange Mode"),
    rEffParTF(Poutsub, 11, rShort("sub"),
              rPreset(4, true), rPreset(7, true), rPreset(9, true),
              rDefault(false), "Output Subtraction"),
};
#undef rBegin
#undef rEnd
#undef rObject

Chorus::Chorus(EffectParams pars)
    :Effect(pars),
      lfo(pars.srate, pars.bufsize),
      maxdelay((int)(MAX_CHORUS_DELAY / 1000.0f * samplerate_f)),
      delaySample(memory.valloc<float>(maxdelay), memory.valloc<float>(maxdelay))
{
    dlk = 0;
    drk = 0;
    setpreset(Ppreset);
    changepar(1, 64);
    lfo.effectlfoout(&lfol, &lfor);
    dl2 = getdelay(lfol);
    dr2 = getdelay(lfor);
    cleanup();
}

Chorus::~Chorus()
{
    memory.devalloc(delaySample.l);
    memory.devalloc(delaySample.r);
}

//get the delay value in samples; xlfo is the current lfo value
float Chorus::getdelay(float xlfo)
{
    float result =
        (Pflangemode) ? 0 : (delay + xlfo * depth) * samplerate_f;

    //check if delay is too big (caused by bad setdelay() and setdepth()
    if((result + 0.5f) >= maxdelay) {
        cerr
        <<
        "WARNING: Chorus.cpp::getdelay(..) too big delay (see setdelay and setdepth funcs.)"
        << endl;
        result = maxdelay - 1.0f;
    }
    return result;
}

//Apply the effect
void Chorus::out(const Stereo<float *> &input)
{
    dl1 = dl2;
    dr1 = dr2;
    lfo.effectlfoout(&lfol, &lfor);

    dl2 = getdelay(lfol);
    dr2 = getdelay(lfor);

    for(int i = 0; i < buffersize; ++i) {
        float inL = input.l[i];
        float inR = input.r[i];
        //LRcross
        Stereo<float> tmpc(inL, inR);
        inL = tmpc.l * (1.0f - lrcross) + tmpc.r * lrcross;
        inR = tmpc.r * (1.0f - lrcross) + tmpc.l * lrcross;

        //Left channel

        //compute the delay in samples using linear interpolation between the lfo delays
        float mdel =
            (dl1 * (buffersize - i) + dl2 * i) / buffersize_f;
        if(++dlk >= maxdelay)
            dlk = 0;
        float tmp = dlk - mdel + maxdelay * 2.0f; //where should I get the sample from

        dlhi  = (int) tmp;
        dlhi %= maxdelay;

        float dlhi2 = (dlhi - 1 + maxdelay) % maxdelay;
        float dllo  = 1.0f + floorf(tmp) - tmp;
        efxoutl[i] = cinterpolate(delaySample.l, maxdelay, dlhi2) * dllo
                     + cinterpolate(delaySample.l, maxdelay,
                                    dlhi) * (1.0f - dllo);
        delaySample.l[dlk] = inL + efxoutl[i] * fb;

        //Right channel

        //compute the delay in samples using linear interpolation between the lfo delays
        mdel = (dr1 * (buffersize - i) + dr2 * i) / buffersize_f;
        if(++drk >= maxdelay)
            drk = 0;
        tmp = drk * 1.0f - mdel + maxdelay * 2.0f; //where should I get the sample from

        dlhi  = (int) tmp;
        dlhi %= maxdelay;

        dlhi2      = (dlhi - 1 + maxdelay) % maxdelay;
        dllo       = 1.0f + floorf(tmp) - tmp;
        efxoutr[i] = cinterpolate(delaySample.r, maxdelay, dlhi2) * dllo
                     + cinterpolate(delaySample.r, maxdelay,
                                    dlhi) * (1.0f - dllo);
        delaySample.r[dlk] = inR + efxoutr[i] * fb;
    }

    if(Poutsub)
        for(int i = 0; i < buffersize; ++i) {
            efxoutl[i] *= -1.0f;
            efxoutr[i] *= -1.0f;
        }

    for(int i = 0; i < buffersize; ++i) {
        efxoutl[i] *= pangainL;
        efxoutr[i] *= pangainR;
    }
}

//Cleanup the effect
void Chorus::cleanup(void)
{
    memset(delaySample.l, 0, maxdelay * sizeof(float));
    memset(delaySample.r, 0, maxdelay * sizeof(float));
}

//Parameter control
void Chorus::setdepth(unsigned char _Pdepth)
{
    Pdepth = _Pdepth;
    depth  = (powf(8.0f, (Pdepth / 127.0f) * 2.0f) - 1.0f) / 1000.0f; //seconds
}

void Chorus::setdelay(unsigned char _Pdelay)
{
    Pdelay = _Pdelay;
    delay  = (powf(10.0f, (Pdelay / 127.0f) * 2.0f) - 1.0f) / 1000.0f; //seconds
}

void Chorus::setfb(unsigned char _Pfb)
{
    Pfb = _Pfb;
    fb  = (Pfb - 64.0f) / 64.1f;
}

void Chorus::setvolume(unsigned char _Pvolume)
{
    Pvolume   = _Pvolume;
    outvolume = Pvolume / 127.0f;
    volume    = (!insertion) ? 1.0f : outvolume;
}

unsigned char Chorus::getpresetpar(unsigned char npreset, unsigned int npar)
{
#define	PRESET_SIZE 12
#define	NUM_PRESETS 10
    static const unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        //Chorus1
        {64, 64, 50, 0,   0, 90, 40,  85, 64,  119, 0, 0},
        //Chorus2
        {64, 64, 45, 0,   0, 98, 56,  90, 64,  19,  0, 0},
        //Chorus3
        {64, 64, 29, 0,   1, 42, 97,  95, 90,  127, 0, 0},
        //Celeste1
        {64, 64, 26, 0,   0, 42, 115, 18, 90,  127, 0, 0},
        //Celeste2
        {64, 64, 29, 117, 0, 50, 115, 9,  31,  127, 0, 1},
        //Flange1
        {64, 64, 57, 0,   0, 60, 23,  3,  62,  0,   0, 0},
        //Flange2
        {64, 64, 33, 34,  1, 40, 35,  3,  109, 0,   0, 0},
        //Flange3
        {64, 64, 53, 34,  1, 94, 35,  3,  54,  0,   0, 1},
        //Flange4
        {64, 64, 40, 0,   1, 62, 12,  19, 97,  0,   0, 0},
        //Flange5
        {64, 64, 55, 105, 0, 24, 39,  19, 17,  0,   0, 1}
    };
    if(npreset < NUM_PRESETS && npar < PRESET_SIZE) {
        return presets[npreset][npar];
    }
    return 0;
}

void Chorus::setpreset(unsigned char npreset)
{
    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n != 128; n++)
        changepar(n, getpresetpar(npreset, n));
    Ppreset = npreset;
}

void Chorus::changepar(int npar, unsigned char value)
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
            setdelay(value);
            break;
        case 8:
            setfb(value);
            break;
        case 9:
            setlrcross(value);
            break;
        case 10:
            Pflangemode = (value > 1) ? 1 : value;
            break;
        case 11:
            Poutsub = (value > 1) ? 1 : value;
            break;
    }
}

unsigned char Chorus::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return lfo.Pfreq;
        case 3:  return lfo.Prandomness;
        case 4:  return lfo.PLFOtype;
        case 5:  return lfo.Pstereo;
        case 6:  return Pdepth;
        case 7:  return Pdelay;
        case 8:  return Pfb;
        case 9:  return Plrcross;
        case 10: return Pflangemode;
        case 11: return Poutsub;
        default: return 0;
    }
}

}
