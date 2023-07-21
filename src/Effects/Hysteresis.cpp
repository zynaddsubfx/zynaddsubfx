/*
  ZynAddSubFX - a software synthesizer

  Hysteresis.cpp - Hysteresis effect
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
#include "Hysteresis.h"


namespace zyn {

#define rObject Hysteresis
#define rBegin [](const char *msg, rtosc::RtData &d) {
#define rEnd }

rtosc::Ports Hysteresis::ports = {
    {"preset::i", rOptions(Hysteresis 1, Hysteresis 2)
                  rProp(parameter)
                  rDoc("Instrument Presets"), 0,
                  rBegin;
                  rObject *o = (rObject*)d.obj;
                  if(rtosc_narguments(msg))
                      o->setpreset(rtosc_argument(msg, 0).i);
                  else
                      d.reply(d.loc, "i", o->Ppreset);
                  rEnd},
    rEffParVol(rDefault(64)),
    rEffParPan(),
    rEffPar(Pdrive,   2, rShort("drive"),
            "Drive of Hysteresis"),
    rEffPar(Premanence,   3, rShort("rem"),
            "Remanence of Hysteresis"),
    rEffPar(Pcoercivity,  4, rShort("coerc"),
            "Coercivity of Hysteresis"),

};
#undef rBegin
#undef rEnd
#undef rObject

Hysteresis::Hysteresis(EffectParams pars)
    :Effect(pars),
      Pvolume(50),
      Premanence(64),
      Pcoercivity(64)
{
    ja_l = memory.alloc<JilesAtherton>(float(Pcoercivity)*8.0f, float(Premanence)/128.0f);
    ja_r = memory.alloc<JilesAtherton>(float(Pcoercivity)*8.0f, float(Premanence)/128.0f);
}

Hysteresis::~Hysteresis()
{
    memory.devalloc(ja_l);
    memory.devalloc(ja_r);
}

//Initialize the delays
void Hysteresis::init(void)
{
    ja_l->init();
    ja_r->init();
}

//Effect output
void Hysteresis::out(const Stereo<float *> &input)
{
    for(int i = 0; i < buffersize; ++i) {
        efxoutl[i] = ja_l->getMagnetization(input.l[i]*float(Pdrive)/32.0f);
        efxoutr[i] = ja_r->getMagnetization(input.r[i]*float(Pdrive)/32.0f);
    }
    //~ printf("-----i: %d\n", buffersize-1);
    //~ printf("input.l[i]: %f\n", input.l[ buffersize-1]);
    //~ printf("efxoutl[i]: %f\n", efxoutl[ buffersize-1]);
}


//Parameter control
void Hysteresis::setvolume(unsigned char _Pvolume)
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
}

void Hysteresis::setdrive(unsigned char _Pdrive)
{
    Pdrive   = _Pdrive;
}

void Hysteresis::setremanence(unsigned char _Premanence)
{
    Premanence   = _Premanence;
    ja_l->setMr(float(Premanence+3)/128.0f);
    ja_r->setMr(float(Premanence+3)/128.0f);
}

void Hysteresis::setcoercivity(unsigned char _Pcoercivity)
{
    Pcoercivity   = _Pcoercivity;
    ja_l->setHc(float((Pcoercivity+2)));
    ja_r->setHc(float((Pcoercivity+2)));
}



unsigned char Hysteresis::getpresetpar(unsigned char npreset, unsigned int npar)
{
#define	PRESET_SIZE 3
#define	NUM_PRESETS 1
    static const unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        {67, 64, 35 }, //Hysteresis 1

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

void Hysteresis::setpreset(unsigned char npreset)
{
    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n != 128; n++)
        changepar(n, getpresetpar(npreset, n));
    Ppreset = npreset;
}

void Hysteresis::changepar(int npar, unsigned char value)
{
    switch(npar) {
        case 0:
            setvolume(value);
            break;
        case 1:
            setpanning(value);
            break;
        case 2:
            setdrive(value);
            break;
        case 3:
            setremanence(value);
            break;
        case 4:
            setcoercivity(value);
            break;

    }
}

unsigned char Hysteresis::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return Pdrive;
        case 3:  return Premanence;
        case 4:  return Pcoercivity;
        default: return 0; // in case of bogus parameter number
    }
}

}
