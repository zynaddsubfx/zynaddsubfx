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
#include "../Misc/Util.h"
#include "Reverse.h"

namespace zyn {

#define rObject Reverse
#define rBegin [](const char *msg, rtosc::RtData &d) {
#define rEnd }

rtosc::Ports Reverse::ports = {
    rPresetForVolume,
    rEffParVol(),
    rEffParPan(),
    rEffPar(Pdelay,   2, rShort("delay"), rLinear(0, 127), rDefault(31),
            "Length of Reversed Segment"),
    rEffParTF(Pstereo,3, rShort("stereo"),
              "Stereo"),
    rEffPar(Pphase,   4, rShort("phase"), rLinear(0, 127), rDefault(64),
            "Phase offset for Reversed Segment"),
    rEffPar(Pcrossfade,5, rShort("fade"), rLinear(0, 127), rDefault(16),
            "Cross Fade Time between Reversed Segments 1/100s"),
    rEffParOpt(Psyncmode,    6, rShort("sync"),
            rOptions(SYNCMODES), rLinear(0,127), 
            "Sync Mode"),
};
#undef rBegin
#undef rEnd
#undef rObject

Reverse::Reverse(EffectParams pars, const AbsTime *time_)
    :Effect(pars),Pvolume(50),Pdelay(31),Pphase(64), Pcrossfade(16), PsyncMode(AUTO), Pstereo(0),time(time_)
{
    float tRef = float(time->time());
    printf("tRef: %f\n", tRef);
    reverterL = memory.alloc<Reverter>(&memory, float(Pdelay+1)/128.0f*MAX_REV_DELAY_SECONDS, samplerate, buffersize, tRef, time);
    reverterR = memory.alloc<Reverter>(&memory, float(Pdelay+1)/128.0f*MAX_REV_DELAY_SECONDS, samplerate, buffersize, tRef, time);
    setpanning(64);
}

Reverse::~Reverse()
{
    memory.dealloc(reverterL);
    memory.dealloc(reverterR);
}

//Cleanup the effect
void Reverse::cleanup(void)
{
    reverterR->reset();
    reverterL->reset();
}


//Effect output
void Reverse::out(const Stereo<float *> &input)
{
    if(Pstereo) //Stereo
        for(int i = 0; i < buffersize; ++i) {
            efxoutl[i] = input.l[i] * pangainL;
            efxoutr[i] = input.r[i] * pangainR;
        }
    else //Mono
        for(int i = 0; i < buffersize; ++i)
            efxoutl[i] = (input.l[i] * pangainL + input.r[i] * pangainR);
    

    
    unsigned int beat_new;
    if (time->tempo && speedfactor)
    {
        tick = (time->beat-1)*1920 + time->tick;
        const unsigned int delay_ticks = int(1920.0f * speedfactor);
        beat_new = tick/delay_ticks;
        const unsigned int phase_ticks = tick%delay_ticks;
        
        if(beat_new!=beat_new_hist) {
            
            const float syncPos = (phase_ticks/delay_ticks)*(60.0f/time->tempo)*(float)(time->samplerate());
            printf("syncPos: %f\n", syncPos);
            reverterL->sync(syncPos);
            if(Pstereo) reverterR->sync(syncPos);
        }
    }
    beat_new_hist = beat_new;
    reverterL->filterout(efxoutl);
    if(Pstereo) reverterR->filterout(efxoutr);
    else memcpy(efxoutr, efxoutl, bufferbytes);
}


//Parameter control
void Reverse::setvolume(unsigned char _Pvolume)
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

void Reverse::setdelay(unsigned char value)
{
    Pdelay = limit(value,static_cast<unsigned char>(0),static_cast<unsigned char>(127));
    reverterL->setdelay(float(Pdelay+1)/128.0f*MAX_REV_DELAY_SECONDS);
    reverterR->setdelay(float(Pdelay+1)/128.0f*MAX_REV_DELAY_SECONDS);
}

void Reverse::setphase(unsigned char value)
{
    Pphase = value;
    reverterL->setphase(float(Pphase)/127.0f);
    reverterR->setphase(float(Pphase)/127.0f);
}

void Reverse::setcrossfade(unsigned char value)
{
    Pcrossfade = value;
    reverterL->setcrossfade(float(value)/100.0f);
    reverterR->setcrossfade(float(value)/100.0f);
}

void Reverse::setsyncMode(unsigned char value)
{
    PsyncMode = value;
    reverterL->setsyncMode((SyncMode)value);
    reverterR->setsyncMode((SyncMode)value);
}

unsigned char Reverse::getpresetpar(unsigned char npreset, unsigned int npar)
{
    return 0;
}

void Reverse::setpreset(unsigned char npreset)
{
    Ppreset = npreset;
}

void Reverse::changepar(int npar, unsigned char value)
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
            Pstereo = (value > 1) ? 1 : value;
            break;
        case 4:
            setphase(value);
            break;
        case 5:
            setcrossfade(value);
            break;
        case 6:
            setsyncMode(value);
            break;
    }
}

unsigned char Reverse::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return Pdelay;
        case 3:  return Pstereo;
        case 4:  return Pphase;
        case 5:  return Pcrossfade;
        case 6:  return PsyncMode;
        default: return 0; // in case of bogus parameter number
    }
}

}
