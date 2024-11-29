/*
  ZynAddSubFX - a software synthesizer

  Reverse.cpp - Reverse Delay Effect
  Copyright (C) 2023-2024 Michael Kirchner
  Author: Michael Kirchner

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include "../DSP/Reverter.h"
#include "../Misc/Allocator.h"
#include "../Misc/Util.h"
#include "../Misc/Time.h"
#include "Reverse.h"

namespace zyn {

#define rObject Reverse
#define rBegin [](const char *msg, rtosc::RtData &d) {
#define rEnd }

rtosc::Ports Reverse::ports = {
    rPresetForVolume,
    rEffParVol(),
    rEffParPan(),
    rEffPar(Pdelay,   2, rShort("delay"), rDefault(31),
            "Length of Reversed Segment"),
    rEffParTF(Pstereo,3, rShort("stereo"),
            "Enable Stereo Processing"),
    rEffPar(Pphase,   4, rShort("phase"), rDefault(64),
            "Phase offset for Reversed Segment"),
    rEffPar(Pcrossfade, 5, rShort("fade"), rDefault(64), rUnit(1\/100 s), 
            "Cross Fade Time between Reversed Segments"),
    rEffParOpt(Psyncmode,    6, rShort("sync"), rDefault(NOTEON),
            rOptions(SYNCMODES), 
            "Sync Mode"),
};
#undef rBegin
#undef rEnd
#undef rObject

Reverse::Reverse(EffectParams pars, const AbsTime *time_)
    :Effect(pars),Pvolume(50),Pdelay(31),Pphase(64), Pcrossfade(64), PsyncMode(NOTEON), Pstereo(0),time(time_)
{
    float tRef = float(time->time());
    reverterL = memory.alloc<Reverter>(&memory, float(Pdelay+1)/128.0f*MAX_REV_DELAY_SECONDS, samplerate, buffersize, tRef, time);
    reverterR = memory.alloc<Reverter>(&memory, float(Pdelay+1)/128.0f*MAX_REV_DELAY_SECONDS, samplerate, buffersize, tRef, time);
    setpanning(64);
    setvolume(Pvolume);
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
    // prepare the processing buffers
    if(Pstereo) //Stereo
        for(int i = 0; i < buffersize; ++i) {
            efxoutl[i] = input.l[i] * pangainL;
            efxoutr[i] = input.r[i] * pangainR;
        }
    else //Mono
        for(int i = 0; i < buffersize; ++i)
            efxoutl[i] = (input.l[i] * pangainL + input.r[i] * pangainR);

    // process external timecode to sync

    unsigned int beat_new = 0;
    if (time->tempo && speedfactor && PsyncMode == HOST)
    {
        tick = (time->beat-1)*PPQ + time->tick;
        const unsigned int delay_ticks = int((float)PPQ * speedfactor);
        beat_new = tick/delay_ticks;
        const unsigned int phase_ticks = tick%delay_ticks;

        if(beat_new!=beat_new_hist) {
            const float syncPos = (phase_ticks/delay_ticks)*(60.0f/time->tempo)*(float)(time->samplerate());
            reverterL->sync(syncPos);
            if(Pstereo) reverterR->sync(syncPos);
        }
    }
    // store beat_new for next cycle
    beat_new_hist = beat_new;

    // do the actual processing
    reverterL->filterout(efxoutl);
    if(Pstereo) reverterR->filterout(efxoutr);
    else memcpy(efxoutr, efxoutl, bufferbytes);
}

void Reverse::update()
{
    // process noteon trigger
    if( (PsyncMode == NOTEON || PsyncMode == NOTEONOFF) ) {
        reverterL->sync(0.0f);
        if(Pstereo) reverterR->sync(0.0f);
    }
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
    (void)npreset;
    (void)npar;
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
