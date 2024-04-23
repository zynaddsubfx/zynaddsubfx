/*
  ZynAddSubFX - a software synthesizer

  Hysteresis.cpp - Hysteresis effect
  Copyright (C) 2024 Michael Kirchner
  Author: Michael Kirchner

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
        rEffParTF(Pstereo, 5, rShort("stereo"),
              , "Stereo"),

};
#undef rBegin
#undef rEnd
#undef rObject

Hysteresis::Hysteresis(EffectParams pars)
    :Effect(pars),
      Pvolume(50),
      Pstereo(1),
      remanence(0.5f),
      coercivity(0.5f)
{

}

Hysteresis::~Hysteresis()
{

}

//Initialize the delays
void Hysteresis::init(void)
{
    state_l = 0.0f;
    state_r = 0.0f;
}

inline float dualCos(float x, float drive, float par)
{
    float y;
    x *= drive;
    if (x > 1.0f)
        y = 1.0f;
    else if (x < -1.0f)
        y = -1.0f;
    else if (fabs(x)<par) 
            y = 0.0f;
        else {
            if (x>0)
            {
                const float smpTmp = (x-par)/(1.0f-par);
                y = 0.5 + 0.5 * cos(smpTmp*PI-PI);
            }
            else
            {
                const float smpTmp = (x+par)/(1.0f-par);
                y = -0.5 - 0.5 * cos(smpTmp*PI-PI);
            }
        }
    return y/drive;
}

void Hysteresis::out(const Stereo<float *> &input)
{
    for(int i = 0; i < buffersize; ++i) {

        state_l += dualCos(input.l[i] - state_l, drive, coercivity);
        if(Pstereo != 0) state_r += dualCos(input.r[i] - state_r, drive, coercivity);
        efxoutl[i] = state_l;
        if(Pstereo != 0) efxoutr[i] = state_r;
    }
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

void Hysteresis::setdrive(unsigned char Pdrive)
{
    drive   = 0.1f + Pdrive / 16.0f;
}

void Hysteresis::setremanence(unsigned char Premanence)
{
    remanence   = Premanence / 1270.0f;

}

void Hysteresis::setcoercivity(unsigned char Pcoercivity)
{
    coercivity   = Pcoercivity / 512.0f;
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
        case 5:
            Pstereo = (value > 1) ? 1 : value;
            break;

    }
}

unsigned char Hysteresis::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return int((drive-0.1)*16.0f);
        case 3:  return int(remanence*1270.0f);
        case 4:  return int(coercivity*512.0f);
        case 5:  return Pstereo;
        default: return 0; // in case of bogus parameter number
    }
}

}
