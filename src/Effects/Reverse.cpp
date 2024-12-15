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
#define HZ2BPM 60.0f // frequency (Hz) * HZ2BPM = frequency (BPM)

#define rObject Reverse
#define rBegin [](const char *msg, rtosc::RtData &d) {
#define rEnd }

rtosc::Ports Reverse::ports = {
    {"preset::i", rProp(parameter)
              rOptions(noteon, noteonoff, auto)
              rProp(alias)
              rShort("preset")
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
    rEffParOpt(Psyncmode,    6, rShort("mode"), rDefault(NOTEON),
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
    if (time->tempo && speedfactor && (PsyncMode == HOST ))
    {
        // calculate the buffer length measured in ticks
        const unsigned int buffer_ticks = (unsigned int)((float)buffersize / (float)samplerate * // seconds *
                                            ((float)time->tempo / HZ2BPM) *                      // beats per seconds * 
                                            (float)PPQ);                                         // pulses per quarter note
        // calculate the tick count at the end of the buffer
        tick = (time->beat-1) * PPQ + time->tick + buffer_ticks; // beat is 1...4
        const unsigned int delay_ticks = (unsigned int)((float)PPQ * speedfactor);
        beat_new = tick/delay_ticks;
        
        // check whether there is a beat inside this buffer
        if(beat_new!=beat_new_hist) {
            // calculate ticks between beat and buffer end
            const unsigned int phase_ticks = buffer_ticks - (tick%delay_ticks);
            // calculate sample offset of the beat
            const float syncPos = (phase_ticks/delay_ticks)*(HZ2BPM/(float)time->tempo)*(float)samplerate;
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
    reverterL->setcrossfade(float(value+1)/100.0f);
    reverterR->setcrossfade(float(value+1)/100.0f);
}

void Reverse::setsyncMode(unsigned char value)
{
    PsyncMode = value;
    reverterL->setsyncMode((SyncMode)value);
    reverterR->setsyncMode((SyncMode)value);
}

unsigned char Reverse::getpresetpar(unsigned char npreset, unsigned int npar)
{
#define	PRESET_SIZE 7
#define	NUM_PRESETS 3
    static const unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        //NOTEON
        {64, 64, 25, 0, 64,   32, NOTEON},
        //NOTEONOFF
        {64, 64, 25, 0, 64,   16, NOTEONOFF},
        //AUTO
        {64, 64, 25, 0, 64,   50, AUTO}
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

void Reverse::setpreset(unsigned char npreset)
{
    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n != 128; n++)
        changepar(n, getpresetpar(npreset, n));
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
