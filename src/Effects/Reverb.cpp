/*
  ZynAddSubFX - a software synthesizer

  Reverb.cpp - Reverberation effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "Reverb.h"
#include "../Misc/Util.h"
#include "../Misc/Allocator.h"
#include "../DSP/AnalogFilter.h"
#include "../DSP/Unison.h"
#include <cmath>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>

namespace zyn {

#define rObject Reverb
#define rBegin [](const char *msg, rtosc::RtData &d) {
#define rEnd }

rtosc::Ports Reverb::ports = {
    {"preset::i", rOptions(Cathedral1, Cathedral2, Cathedral3,
            Hall1, Hall2, Room1, Room2, Basement,
            Tunnel, Echoed1, Echoed2, VeryLong1, VeryLong2)
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
    rEffParVol(rDefaultDepends(presetOfVolume),
          rDefault(90),
          rPresets(80, 80, 80),
          rPresetsAt(5, 100, 100, 110, 85, 95),
          rPresetsAt(16, 40, 40, 40, 45, 45),
          rPresetsAt(21, 50, 50, 55, 42, 47, 45, 45, 45)),
    rEffParPan(rDefaultDepends(preset), rPreset(8, 80)),
    rEffPar(Ptime,    2, rShort("time"), rLinear(0, 127),
            rPresets(63, 69, 69, 51, 53, 33, 21, 14, 84, 26, 40, 93, 111),
            "Length of Reverb"),
    rEffPar(Pidelay,  3, rShort("i.time"),
            rPresets(24, 35, 24, 10, 20, 0, 26, 0, 20, 60, 88, 15, 30),
            "Delay for first impulse"),
    rEffPar(Pidelayfb,4, rShort("i.fb"), rPresetsAt(8, 42, 71, 71), rDefault(0),
            "Feedback for first impulse"),
    rEffPar(Plpf,     7, rShort("lpf"),
            rPreset(0, 85), rPresetsAt(6, 62, 127, 51, 114, 114, 114, 114),
            rDefault(127), "Low pass filter"),
    rEffPar(Phpf,     8, rShort("hpf"),
            rPresets(5), rPresetsAt(2, 75, 21, 75), rPreset(7, 5),
            rPreset(12, 90), rDefault(0), "High pass filter"),
    rEffPar(Plohidamp,9, rShort("damp"),
            rPresets(83, 71, 78, 78, 71, 106, 77, 71, 78, 64, 88, 77, 74),
            "Dampening"),
    //Todo make this a selector
    rEffParOpt(Ptype,    10, rShort("type"),
            rOptions(Random, Freeverb, Bandwidth),
            rPresets(Freeverb, Random, Freeverb, Freeverb, Freeverb, Random,
                     Freeverb, Random, Freeverb, Freeverb, Freeverb, Random,
                     Freeverb)
            rDefault(Random), "Type"),
    rEffPar(Proomsize,11,rShort("size"),
            rPreset(2, 85), rPresetsAt(5, 30, 45, 25, 105),
            rPresetsAt(11, 95, 80), rDefault(64),
            "Room Size"),
    rEffPar(Pbandwidth,12,rShort("bw"), rDefault(20), "Bandwidth"),
};
#undef rBegin
#undef rEnd
#undef rObject

Reverb::Reverb(EffectParams pars)
    :Effect(pars),
      // defaults
      Pvolume(48),
      Ptime(64),
      Pidelay(40),
      Pidelayfb(0),
      Plpf(127),
      Phpf(0),
      Plohidamp(80),
      Ptype(1),
      Proomsize(64),
      Pbandwidth(30),
      idelaylen(0),
      roomsize(1.0f),
      rs(1.0f),
      bandwidth(NULL),
      idelay(NULL),
      lpf(NULL),
      hpf(NULL) // no filter
{
    for(int i = 0; i < REV_COMBS * 2; ++i) {
        comblen[i] = 800 + (int)(RND * 1400.0f);
        combk[i]   = 0;
        lpcomb[i]  = 0;
        combfb[i]  = -0.97f;
        comb[i]    = NULL;
    }

    for(int i = 0; i < REV_APS * 2; ++i) {
        aplen[i] = 500 + (int)(RND * 500.0f);
        apk[i]   = 0;
        ap[i]    = NULL;
    }
    setpreset(Ppreset);
    cleanup(); //do not call this before the comb initialisation
}


Reverb::~Reverb()
{
    memory.devalloc(idelay);
    memory.dealloc(hpf);
    memory.dealloc(lpf);

    for(int i = 0; i < REV_APS * 2; ++i)
        memory.devalloc(ap[i]);
    for(int i = 0; i < REV_COMBS * 2; ++i)
        memory.devalloc(comb[i]);

    memory.dealloc(bandwidth);
}

//Cleanup the effect
void Reverb::cleanup(void)
{
    for(int i = 0; i < REV_COMBS * 2; ++i) {
        lpcomb[i] = 0.0f;
        for(int j = 0; j < comblen[i]; ++j)
            comb[i][j] = 0.0f;
    }

    for(int i = 0; i < REV_APS * 2; ++i)
        for(int j = 0; j < aplen[i]; ++j)
            ap[i][j] = 0.0f;

    if(idelay)
        for(int i = 0; i < idelaylen; ++i)
            idelay[i] = 0.0f;
    if(hpf)
        hpf->cleanup();
    if(lpf)
        lpf->cleanup();
}

//Process one channel; 0=left, 1=right
void Reverb::processmono(int ch, float *output, float *inputbuf)
{
    //todo: implement the high part from lohidamp

    for(int j = REV_COMBS * ch; j < REV_COMBS * (ch + 1); ++j) {
        int &ck = combk[j];
        const int comblength = comblen[j];
        float    &lpcombj    = lpcomb[j];

        for(int i = 0; i < buffersize; ++i) {
            float fbout = comb[j][ck] * combfb[j];
            fbout   = fbout * (1.0f - lohifb) + lpcombj * lohifb;
            lpcombj = fbout;

            comb[j][ck] = inputbuf[i] + fbout;
            output[i]  += fbout;

            if((++ck) >= comblength)
                ck = 0;
        }
    }

    for(int j = REV_APS * ch; j < REV_APS * (1 + ch); ++j) {
        int &ak = apk[j];
        const int aplength = aplen[j];
        for(int i = 0; i < buffersize; ++i) {
            float tmp = ap[j][ak];
            ap[j][ak] = 0.7f * tmp + output[i];
            output[i] = tmp - 0.7f * ap[j][ak];
            if((++ak) >= aplength)
                ak = 0;
        }
    }
}

//Effect output
void Reverb::out(const Stereo<float *> &smp)
{
    if(!Pvolume && insertion)
        return;

    float inputbuf[buffersize];
    for(int i = 0; i < buffersize; ++i)
        inputbuf[i] = (smp.l[i] + smp.r[i]) / 2.0f;

    if(idelay)
        for(int i = 0; i < buffersize; ++i) {
            //Initial delay r
            float tmp = inputbuf[i] + idelay[idelayk] * idelayfb;
            inputbuf[i]     = idelay[idelayk];
            idelay[idelayk] = tmp;
            idelayk++;
            if(idelayk >= idelaylen)
                idelayk = 0;
        }

    if(bandwidth)
        bandwidth->process(buffersize, inputbuf);

    if(lpf)
        lpf->filterout(inputbuf);
    if(hpf)
        hpf->filterout(inputbuf);

    processmono(0, efxoutl, inputbuf); //left
    processmono(1, efxoutr, inputbuf); //right

    float lvol = rs / REV_COMBS * pangainL;
    float rvol = rs / REV_COMBS * pangainR;
    if(insertion != 0) {
        lvol *= 2.0f;
        rvol *= 2.0f;
    }
    for(int i = 0; i < buffersize; ++i) {
        efxoutl[i] *= lvol;
        efxoutr[i] *= rvol;
    }
}


//Parameter control
void Reverb::setvolume(unsigned char _Pvolume)
{
    Pvolume = _Pvolume;
    if(!insertion) {
        if (Pvolume == 0) {
            outvolume = 0.0f;
        } else {
            outvolume = powf(0.01f, (1.0f - Pvolume / 127.0f)) * 4.0f;
        }
        volume    = 1.0f;
    }
    else {
        volume = outvolume = Pvolume / 127.0f;
        if(Pvolume == 0)
            cleanup();
    }
}

void Reverb::settime(unsigned char _Ptime)
{
    Ptime = _Ptime;
    float t = powf(60.0f, Ptime / 127.0f) - 0.97f;

    for(int i = 0; i < REV_COMBS * 2; ++i)
        combfb[i] =
            -expf((float)comblen[i] / samplerate_f * logf(0.001f) / t);
    //the feedback is negative because it removes the DC
}

void Reverb::setlohidamp(unsigned char _Plohidamp)
{
    Plohidamp = (_Plohidamp < 64) ? 64 : _Plohidamp;
    //remove this when the high part from lohidamp is added
    if(Plohidamp == 64) {
        lohidamptype = 0;
        lohifb = 0.0f;
    }
    else {
        if(Plohidamp < 64)
            lohidamptype = 1;
        if(Plohidamp > 64)
            lohidamptype = 2;
        float x = fabsf((float)(Plohidamp - 64) / 64.1f);
        lohifb = x * x;
    }
}

void Reverb::setidelay(unsigned char _Pidelay)
{
    Pidelay = _Pidelay;
    float delay = powf(50.0f * Pidelay / 127.0f, 2.0f) - 1.0f;
    int newDelayLen = (int) (samplerate_f * delay / 1000);
    if(newDelayLen == idelaylen)
        return;

    memory.devalloc(idelay);

    idelaylen = newDelayLen;
    if(idelaylen > 1) {
        idelayk = 0;
        idelay  = memory.valloc<float>(idelaylen);
        memset(idelay, 0, idelaylen * sizeof(float));
    }
}

void Reverb::setidelayfb(unsigned char _Pidelayfb)
{
    Pidelayfb = _Pidelayfb;
    idelayfb  = Pidelayfb / 128.0f;
}

void Reverb::sethpf(unsigned char _Phpf)
{
    Phpf = _Phpf;
    if(Phpf == 0) { //No HighPass
        memory.dealloc(hpf);
    } else {
        float fr = expf(sqrtf(Phpf / 127.0f) * logf(10000.0f)) + 20.0f;
        if(hpf == NULL)
            hpf = memory.alloc<AnalogFilter>(3, fr, 1, 0, samplerate, buffersize);
        else
            hpf->setfreq(fr);
    }
}

void Reverb::setlpf(unsigned char _Plpf)
{
    Plpf = _Plpf;
    if(Plpf == 127) { //No LowPass
        memory.dealloc(lpf);
    } else {
        float fr = expf(sqrtf(Plpf / 127.0f) * logf(25000.0f)) + 40.0f;
        if(!lpf)
            lpf = memory.alloc<AnalogFilter>(2, fr, 1, 0, samplerate, buffersize);
        else
            lpf->setfreq(fr);
    }
}

void Reverb::settype(unsigned char _Ptype)
{
    Ptype = _Ptype;
    const int NUM_TYPES = 3;
    const int combtunings[NUM_TYPES][REV_COMBS] = {
        //this is unused (for random)
        {0,    0,    0,    0,    0,    0,    0,    0      },
        //Freeverb by Jezar at Dreampoint
        {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617   },
        //duplicate of Freeverb by Jezar at Dreampoint
        {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617   }
    };

    const int aptunings[NUM_TYPES][REV_APS] = {
        //this is unused (for random)
        {0,   0,   0,   0    },
        //Freeverb by Jezar at Dreampoint
        {225, 341, 441, 556  },
        //duplicate of Freeverb by Jezar at Dreampoint
        {225, 341, 441, 556  }
    };

    if(Ptype >= NUM_TYPES)
        Ptype = NUM_TYPES - 1;

    // adjust the combs according to the samplerate
    float samplerate_adjust = samplerate_f / 44100.0f;
    float tmp;
    for(int i = 0; i < REV_COMBS * 2; ++i) {
        if(Ptype == 0)
            tmp = 800.0f + (int)(RND * 1400.0f);
        else
            tmp = combtunings[Ptype][i % REV_COMBS];
        tmp *= roomsize;
        if(i > REV_COMBS)
            tmp += 23.0f;
        tmp *= samplerate_adjust; //adjust the combs according to the samplerate
        if(tmp < 10.0f)
            tmp = 10.0f;
        combk[i]   = 0;
        lpcomb[i]  = 0;
        if(comblen[i] != (int)tmp || comb[i] == NULL) {
            comblen[i] = (int) tmp;
            memory.devalloc(comb[i]);
            comb[i] = memory.valloc<float>(comblen[i]);
        }
    }

    for(int i = 0; i < REV_APS * 2; ++i) {
        if(Ptype == 0)
            tmp = 500 + (int)(RND * 500.0f);
        else
            tmp = aptunings[Ptype][i % REV_APS];
        tmp *= roomsize;
        if(i > REV_APS)
            tmp += 23.0f;
        tmp *= samplerate_adjust; //adjust the combs according to the samplerate
        if(tmp < 10)
            tmp = 10;
        apk[i]   = 0;
        if(aplen[i] != (int)tmp || ap[i] == NULL) {
            aplen[i] = (int) tmp;
            memory.devalloc(ap[i]);
            ap[i] = memory.valloc<float>(aplen[i]);
        }
    }
    memory.dealloc(bandwidth);
    if(Ptype == 2) { //bandwidth
        //TODO the size of the unison buffer may be too small, though this has
        //not been verified yet.
        //As this cannot be resized in a RT context, a good upper bound should
        //be found
        bandwidth = memory.alloc<Unison>(&memory, buffersize / 4 + 1, 2.0f, samplerate_f);
        bandwidth->setSize(50);
        bandwidth->setBaseFrequency(1.0f);
    }
    settime(Ptime);
    cleanup();
}

void Reverb::setroomsize(unsigned char _Proomsize)
{
    Proomsize = _Proomsize;
    if(!Proomsize)
        this->Proomsize = 64;  //this is because the older versions consider roomsize=0
    roomsize = (this->Proomsize - 64.0f) / 64.0f;
    if(roomsize > 0.0f)
        roomsize *= 2.0f;
    roomsize = powf(10.0f, roomsize);
    rs = sqrtf(roomsize);
    settype(Ptype);
}

void Reverb::setbandwidth(unsigned char _Pbandwidth)
{
    Pbandwidth = _Pbandwidth;
    float v = Pbandwidth / 127.0f;
    if(bandwidth)
        bandwidth->setBandwidth(powf(v, 2.0f) * 200.0f);
}

unsigned char Reverb::getpresetpar(unsigned char npreset, unsigned int npar)
{
#define	PRESET_SIZE 13
#define	NUM_PRESETS 13
    static const unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        //Cathedral1
        {80,  64, 63,  24, 0,  0, 0, 85,  5,  83,  1, 64,  20},
        //Cathedral2
        {80,  64, 69,  35, 0,  0, 0, 127, 0,  71,  0, 64,  20},
        //Cathedral3
        {80,  64, 69,  24, 0,  0, 0, 127, 75, 78,  1, 85,  20},
        //Hall1
        {90,  64, 51,  10, 0,  0, 0, 127, 21, 78,  1, 64,  20},
        //Hall2
        {90,  64, 53,  20, 0,  0, 0, 127, 75, 71,  1, 64,  20},
        //Room1
        {100, 64, 33,  0,  0,  0, 0, 127, 0,  106, 0, 30,  20},
        //Room2
        {100, 64, 21,  26, 0,  0, 0, 62,  0,  77,  1, 45,  20},
        //Basement
        {110, 64, 14,  0,  0,  0, 0, 127, 5,  71,  0, 25,  20},
        //Tunnel
        {85,  80, 84,  20, 42, 0, 0, 51,  0,  78,  1, 105, 20},
        //Echoed1
        {95,  64, 26,  60, 71, 0, 0, 114, 0,  64,  1, 64,  20},
        //Echoed2
        {90,  64, 40,  88, 71, 0, 0, 114, 0,  88,  1, 64,  20},
        //VeryLong1
        {90,  64, 93,  15, 0,  0, 0, 114, 0,  77,  0, 95,  20},
        //VeryLong2
        {90,  64, 111, 30, 0,  0, 0, 114, 90, 74,  1, 80,  20}
    };
    if(npreset < NUM_PRESETS && npar < PRESET_SIZE) {
        if (npar == 0 && insertion != 0) {
            /* lower the volume if reverb is insertion effect */
            return presets[npreset][npar] / 2;
        }
        return presets[npreset][npar];
    }
    return 0;
}

void Reverb::setpreset(unsigned char npreset)
{
    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n != 128; n++)
        changepar(n, getpresetpar(npreset, n));
    Ppreset = npreset;
}

void Reverb::changepar(int npar, unsigned char value)
{
    switch(npar) {
        case 0:
            setvolume(value);
            break;
        case 1:
            setpanning(value);
            break;
        case 2:
            settime(value);
            break;
        case 3:
            setidelay(value);
            break;
        case 4:
            setidelayfb(value);
            break;
//      case 5:
//          setrdelay(value);
//          break;
//      case 6:
//          seterbalance(value);
//          break;
        case 7:
            setlpf(value);
            break;
        case 8:
            sethpf(value);
            break;
        case 9:
            setlohidamp(value);
            break;
        case 10:
            settype(value);
            break;
        case 11:
            setroomsize(value);
            break;
        case 12:
            setbandwidth(value);
            break;
    }
}

unsigned char Reverb::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return Ptime;
        case 3:  return Pidelay;
        case 4:  return Pidelayfb;
        case 7:  return Plpf;
        case 8:  return Phpf;
        case 9:  return Plohidamp;
        case 10: return Ptype;
        case 11: return Proomsize;
        case 12: return Pbandwidth;
        default: return 0;
    }
}

}
