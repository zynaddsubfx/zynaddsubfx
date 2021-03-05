/*
  ZynAddSubFX - a software synthesizer

  Distorsion.cpp - Distorsion effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "Distorsion.h"
#include "../DSP/AnalogFilter.h"
#include "../Misc/WaveShapeSmps.h"
#include "../Misc/Allocator.h"
#include <cmath>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>

namespace zyn {

#define rObject Distorsion
#define rBegin [](const char *msg, rtosc::RtData &d) {
#define rEnd }

rtosc::Ports Distorsion::ports = {
    {"preset::i", rProp(parameter)
                  rOptions(Overdrive 1, Overdrive 2, A. Exciter 1, A. Exciter 2, Guitar Amp,
                    Quantisize)
                  rDoc("Instrument Presets"), 0,
                  rBegin;
                  rObject *o = (rObject*)d.obj;
                  if(rtosc_narguments(msg))
                      o->setpreset(rtosc_argument(msg, 0).i);
                  else
                      d.reply(d.loc, "i", o->Ppreset);
                  rEnd},
    rEffParVol(rDefault(127), rPresetsAt(2, 64, 64)),
    rEffParPan(),
    rEffPar(Plrcross, 2, rShort("l/r"), rDefault(35), "Left/Right Crossover"),
    rEffPar(Pdrive,   3, rShort("drive"),
            rLinear(0, 127), rPresets(56, 29, 75, 85, 63, 88),
            "Input amplification"),
    rEffPar(Plevel,   4, rShort("output"), rPresets(70, 75, 80, 62, 75, 75),
            "Output amplification"),
    rEffParOpt(Ptype,    5, rShort("type"),
            rOptions(Arctangent, Asymmetric, Pow, Sine, Quantisize,
                     Zigzag, Limiter, Upper Limiter, Lower Limiter,
                     Inverse Limiter, Clip, Asym2, Pow2, Sigmoid, Tanh,
                     Cubic, Square), rLinear(0,127), 
            rPresets(Arctangent, Asymmetric, Zigzag,
                     Asymmetric, Pow, Quantisize),
            "Distortion Shape"),
    rEffParTF(Pnegate, 6, rShort("neg"), rDefault(false), "Negate Signal"),
    rEffPar(Plpf, 7, rShort("lpf"),
            rPreset(0, 96), rPreset(4, 55), rDefault(127), "Low Pass Cutoff"),
    rEffPar(Phpf, 8, rShort("hpf"),
            rPreset(2, 105), rPreset(3, 118), rDefault(0), "High Pass Cutoff"),
    rEffParTF(Pstereo, 9, rShort("stereo"),
              rPresets(false, false, true, true, false, true), "Stereo"),
    rEffParTF(Pprefiltering, 10, rShort("p.filt"), rDefault(false),
              "Filtering before/after non-linearity"),
    rEffPar(Pfuncpar,   11, rShort("shape"), rDefault(32),
            rLinear(0, 127), "Shape of the wave shaping function"),
    rEffPar(Poffset,   12, rShort("offset"), rDefault(64),
            rLinear(0, 127), "Input DC Offset"),
    {"waveform:", 0, 0, [](const char *, rtosc::RtData &d)
        {
            Distorsion  &dd = *(Distorsion*)d.obj;
            float        buffer[128], orig[128];
            rtosc_arg_t  args[128];
            char         arg_str[128+1] = {};

            for(int i=0; i<128; ++i)
                buffer[i] = 2*(i/128.0)-1;
            memcpy(orig, buffer, sizeof(float_t)*128);

            waveShapeSmps(sizeof(buffer)/sizeof(buffer[0]), buffer,
                    dd.Ptype + 1, dd.Pdrive, dd.Poffset, dd.Pfuncpar);

            for(int i=0; i<128; ++i) {
                arg_str[i] = 'f';
                args[i].f  = (dd.Pvolume * buffer[i] + (127 - dd.Pvolume) * orig[i]) / 127.0f;
            }

            d.replyArray(d.loc, arg_str, args);
        }},
};
#undef rBegin
#undef rEnd
#undef rObject

Distorsion::Distorsion(EffectParams pars)
    :Effect(pars),
      Pvolume(50),
      Pdrive(90),
      Plevel(64),
      Ptype(0),
      Pnegate(0),
      Plpf(127),
      Phpf(0),
      Pstereo(0),
      Pprefiltering(0),
      Pfuncpar(32),
      Poffset(64)
{
    lpfl = memory.alloc<AnalogFilter>(2, 22000, 1, 0, pars.srate, pars.bufsize);
    lpfr = memory.alloc<AnalogFilter>(2, 22000, 1, 0, pars.srate, pars.bufsize);
    hpfl = memory.alloc<AnalogFilter>(3, 20, 1, 0, pars.srate, pars.bufsize);
    hpfr = memory.alloc<AnalogFilter>(3, 20, 1, 0, pars.srate, pars.bufsize);
    setpreset(Ppreset);
    cleanup();
}

Distorsion::~Distorsion()
{
    memory.dealloc(lpfl);
    memory.dealloc(lpfr);
    memory.dealloc(hpfl);
    memory.dealloc(hpfr);
}

//Cleanup the effect
void Distorsion::cleanup(void)
{
    lpfl->cleanup();
    hpfl->cleanup();
    lpfr->cleanup();
    hpfr->cleanup();
}


//Apply the filters
void Distorsion::applyfilters(float *efxoutl, float *efxoutr)
{
    if(Plpf!=127) lpfl->filterout(efxoutl);
    if(Phpf!=0) hpfl->filterout(efxoutl);
    if(Pstereo != 0) { //stereo
        if(Plpf!=127) lpfr->filterout(efxoutr);
        if(Phpf!=0) hpfr->filterout(efxoutr);
    }
}


//Effect output
void Distorsion::out(const Stereo<float *> &smp)
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

    if(Pprefiltering)
        applyfilters(efxoutl, efxoutr);

    waveShapeSmps(buffersize, efxoutl, Ptype + 1, Pdrive, Poffset, Pfuncpar);
    if(Pstereo)
        waveShapeSmps(buffersize, efxoutr, Ptype + 1, Pdrive, Poffset, Pfuncpar);

    if(!Pprefiltering)
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
void Distorsion::setvolume(unsigned char _Pvolume)
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

void Distorsion::setlpf(unsigned char _Plpf)
{
    Plpf = _Plpf;
    float fr = expf(sqrtf(Plpf / 127.0f) * logf(25000.0f)) + 40.0f;
    lpfl->setfreq(fr);
    lpfr->setfreq(fr);
}

void Distorsion::sethpf(unsigned char _Phpf)
{
    Phpf = _Phpf;
    float fr = expf(sqrtf(Phpf / 127.0f) * logf(25000.0f)) + 20.0f;
    hpfl->setfreq(fr);
    hpfr->setfreq(fr);
}

unsigned char Distorsion::getpresetpar(unsigned char npreset, unsigned int npar)
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

void Distorsion::setpreset(unsigned char npreset)
{
    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n != 128; n++)
        changepar(n, getpresetpar(npreset, n));
    Ppreset = npreset;
    cleanup();
}

void Distorsion::changepar(int npar, unsigned char value)
{
    switch(npar) {
        case 0:
            setvolume(value);
            break;
        case 1:
            setpanning(value);
            break;
        case 2:
            setlrcross(value);
            break;
        case 3:
            Pdrive = value;
            break;
        case 4:
            Plevel = value;
            break;
        case 5:
            if(value > 16)
                Ptype = 16;  //this must be increased if more distorsion types are added
            else
                Ptype = value;
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
            Pstereo = (value > 1) ? 1 : value;
            break;
        case 10:
            Pprefiltering = value;
            break;
        case 11:
            Pfuncpar = value;
            break;
        case 12:
            Poffset = value;
            break;
    }
}

unsigned char Distorsion::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return Plrcross;
        case 3:  return Pdrive;
        case 4:  return Plevel;
        case 5:  return Ptype;
        case 6:  return Pnegate;
        case 7:  return Plpf;
        case 8:  return Phpf;
        case 9:  return Pstereo;
        case 10: return Pprefiltering;
        case 11: return Pfuncpar;
        case 12: return Poffset;
        default: return 0; //in case of bogus parameter number
    }
}

}
