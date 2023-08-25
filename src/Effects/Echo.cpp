/*
  ZynAddSubFX - a software synthesizer

  Echo.cpp - Echo effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Copyright (C) 2009-2010 Mark McCurry
  Author: Nasca Octavian Paul
          Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include "../Misc/Allocator.h"
#include "Echo.h"

#define MAX_DELAY 2

namespace zyn {

#define rObject Echo
#define rBegin [](const char *msg, rtosc::RtData &d) {
#define rEnd }

rtosc::Ports Echo::ports = {
    {"preset::i", rOptions(Echo 1, Echo 2, Echo 3, Simple Echo, Canyon, Panning Echo 1,
                           Panning Echo 2, Panning Echo 3, Feedback Echo)
                  rDefault(0)
                  rProp(alias)
                  rProp(parameter)
                  rDoc("Instrument Presets"), 0,
                  rBegin;
                  rObject *o = (rObject*)d.obj;
                  if(rtosc_narguments(msg))
                      o->setpreset(rtosc_argument(msg, 0).i);
                  else
                      d.reply(d.loc, "i", o->Ppreset);
                  rEnd},
    rPresetForVolume,
    rEffParVol(rDefaultDepends(presetOfVolume), rDefault(67),
        rPresetsAt(6, 81, 81, 62),
        rPresetsAt(16, 33, 33, 33, 33, 33, 33, 40, 40, 31)),
    rEffParPan(rDefaultDepends(preset), rPresetsAt(2, 75, 60, 60, 64, 60, 60)),
    rEffPar(Pdelay,   2, rShort("delay"), rLinear(0, 127),
            rPresets(35, 21, 60, 44, 102, 44, 46, 26, 28),
            "Length of Echo"),
    rEffPar(Plrdelay, 3, rShort("lr delay"),
            rPresetsAt(4, 50, 17, 118, 100, 64), rDefault(64),
            "Difference In Left/Right Delay"),
    rEffPar(Plrcross, 4, rShort("cross"),
            rPresetsAt(5, 0, 100, 127, 100), rDefault(30),
            "Left/Right Crossover"),
    rEffPar(Pfb,      5, rShort("feedback"),
            rPresets(59, 59, 59, 0, 82, 82, 68, 67, 90),
            "Echo Feedback"),
    rEffPar(Phidamp,  6, rShort("damp"),
            rPresets(0, 0, 10, 0, 48, 24, 18, 36, 55),
            "Dampen High Frequencies"),
};
#undef rBegin
#undef rEnd
#undef rObject

Echo::Echo(EffectParams pars)
    :Effect(pars),
      Pvolume(50),
      Pdelay(60),
      Plrdelay(100),
      Pfb(40),
      Phidamp(60),
      delayTime(1),
      lrdelay(0),
      avgDelay(0),
      delay(memory.valloc<float>(MAX_DELAY * pars.srate),
            memory.valloc<float>(MAX_DELAY * pars.srate)),
      old(0.0f),
      pos(0),
      delta(1),
      ndelta(1)
{
    initdelays();
    setpreset(Ppreset);
}

Echo::~Echo()
{
    memory.devalloc(delay.l);
    memory.devalloc(delay.r);
}

//Cleanup the effect
void Echo::cleanup(void)
{
    memset(delay.l, 0, MAX_DELAY * samplerate * sizeof(float));
    memset(delay.r, 0, MAX_DELAY * samplerate * sizeof(float));
    old = Stereo<float>(0.0f);
}

inline int max(int a, int b)
{
    return a > b ? a : b;
}

//Initialize the delays
void Echo::initdelays(void)
{
    cleanup();
    //number of seconds to delay left chan
    float dl = avgDelay - lrdelay;

    //number of seconds to delay right chan
    float dr = avgDelay + lrdelay;

    ndelta.l = max(1, (int) (dl * samplerate));
    ndelta.r = max(1, (int) (dr * samplerate));
    delta = ndelta;
}

//Effect output
void Echo::out(const Stereo<float *> &input)
{
    for(int i = 0; i < buffersize; ++i) {
        float ldl = delay.l[pos.l];
        float rdl = delay.r[pos.r];
        ldl = ldl * (1.0f - lrcross) + rdl * lrcross;
        rdl = rdl * (1.0f - lrcross) + ldl * lrcross;

        efxoutl[i] = ldl * 2.0f;
        efxoutr[i] = rdl * 2.0f;

        ldl = input.l[i] * pangainL - ldl * fb;
        rdl = input.r[i] * pangainR - rdl * fb;

        //LowPass Filter
        old.l = delay.l[(pos.l + delta.l) % (MAX_DELAY * samplerate)] =
                    ldl * hidamp + old.l * (1.0f - hidamp);
        old.r = delay.r[(pos.r + delta.r) % (MAX_DELAY * samplerate)] =
                    rdl * hidamp + old.r * (1.0f - hidamp);

        //increment
        ++pos.l; // += delta.l;
        ++pos.r; // += delta.r;

        //ensure that pos is still in bounds
        pos.l %= MAX_DELAY * samplerate;
        pos.r %= MAX_DELAY * samplerate;

        //adjust delay if needed
        delta.l = (15 * delta.l + ndelta.l) / 16;
        delta.r = (15 * delta.r + ndelta.r) / 16;
    }
}


//Parameter control
void Echo::setvolume(unsigned char _Pvolume)
{
    Pvolume = _Pvolume;

    if(insertion == 0) {
        if (Pvolume == 0) {
            outvolume = 0.0f;
        } else {
            outvolume = powf(0.01f, (1.0f - Pvolume / 127.0f)) * 4.0f;
        }
        volume    = 1.0f;
    }
    else
        volume = outvolume = Pvolume / 127.0f;
    if(Pvolume == 0)
        cleanup();
}

void Echo::setdelay(unsigned char _Pdelay)
{
    Pdelay   = _Pdelay;
    avgDelay = (Pdelay / 127.0f * 1.5f); //0 .. 1.5 sec
    initdelays();
}

void Echo::setlrdelay(unsigned char _Plrdelay)
{
    float tmp;
    Plrdelay = _Plrdelay;
    tmp      =
        (powf(2.0f, fabsf(Plrdelay - 64.0f) / 64.0f * 9.0f) - 1.0f) / 1000.0f;
    if(Plrdelay < 64.0f)
        tmp = -tmp;
    lrdelay = tmp;
    initdelays();
}

void Echo::setfb(unsigned char _Pfb)
{
    Pfb = _Pfb;
    fb  = Pfb / 128.0f;
}

void Echo::sethidamp(unsigned char _Phidamp)
{
    Phidamp = _Phidamp;
    hidamp  = 1.0f - Phidamp / 127.0f;
}

unsigned char Echo::getpresetpar(unsigned char npreset, unsigned int npar)
{
#define	PRESET_SIZE 7
#define	NUM_PRESETS 9
    static const unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        {67, 64, 35,  64,  30,  59, 0 }, //Echo 1
        {67, 64, 21,  64,  30,  59, 0 }, //Echo 2
        {67, 75, 60,  64,  30,  59, 10}, //Echo 3
        {67, 60, 44,  64,  30,  0,  0 }, //Simple Echo
        {67, 60, 102, 50,  30,  82, 48}, //Canyon
        {67, 64, 44,  17,  0,   82, 24}, //Panning Echo 1
        {81, 60, 46,  118, 100, 68, 18}, //Panning Echo 2
        {81, 60, 26,  100, 127, 67, 36}, //Panning Echo 3
        {62, 64, 28,  64,  100, 90, 55}  //Feedback Echo
    };
    if(npreset < NUM_PRESETS && npar < PRESET_SIZE) {
        if(npar == 0 && insertion != 0) {
            /* lower the volume if this is insertion effect */
            return presets[npreset][npar] / 2;
        }
        return presets[npreset][npar];
    }
    return 0;
}

void Echo::setpreset(unsigned char npreset)
{
    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n != 128; n++)
        changepar(n, getpresetpar(npreset, n));
    Ppreset = npreset;
}

void Echo::changepar(int npar, unsigned char value)
{
    switch(npar) {
        case 0:
            setvolume(value);
            break;
        case 1:
            setpanning(value);
            break;
        case 2:
            setdelay(value);
            break;
        case 3:
            setlrdelay(value);
            break;
        case 4:
            setlrcross(value);
            break;
        case 5:
            setfb(value);
            break;
        case 6:
            sethidamp(value);
            break;
    }
}

unsigned char Echo::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return Pdelay;
        case 3:  return Plrdelay;
        case 4:  return Plrcross;
        case 5:  return Pfb;
        case 6:  return Phidamp;
        default: return 0; // in case of bogus parameter number
    }
}

}
