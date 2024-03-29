/*
  ZynAddSubFX - a software synthesizer

  Reverb.h - Reverberation effect
  Copyright (C) 2002-2009 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef REVERB_H
#define REVERB_H

#include "Effect.h"

#define REV_COMBS 8
#define REV_APS 4

namespace zyn {

/**Creates Reverberation Effects*/
class Reverb final:public Effect
{
    public:
        Reverb(EffectParams pars);
        ~Reverb();
        void out(const Stereo<float *> &smp);
        void cleanup(void);

        unsigned char getpresetpar(unsigned char npreset, unsigned int npar);
        void setpreset(unsigned char npreset);
        void changepar(int npar, unsigned char value);
        unsigned char getpar(int npar) const;

        static rtosc::Ports ports;
    private:
        //Parametrii
        unsigned char Pvolume;
        unsigned char Ptime;        //duration
        unsigned char Pidelay;      //initial delay
        unsigned char Pidelayfb;    //initial feedback
        unsigned char Plpf;
        unsigned char Phpf;
        unsigned char Plohidamp;    //Low/HighFrequency Damping
        unsigned char Ptype;        //reverb type
        unsigned char Proomsize;    //room size
        unsigned char Pbandwidth;   //bandwidth

        //parameter control
        void setvolume(unsigned char _Pvolume);
        void settime(unsigned char _Ptime);
        void setlohidamp(unsigned char _Plohidamp);
        void setidelay(unsigned char _Pidelay);
        void setidelayfb(unsigned char _Pidelayfb);
        void sethpf(unsigned char _Phpf);
        void setlpf(unsigned char _Plpf);
        void settype(unsigned char _Ptype);
        void setroomsize(unsigned char _Proomsize);
        void setbandwidth(unsigned char _Pbandwidth);
        void processmono(int ch, float *output, float *inputbuf);


        //Parameters
        int   lohidamptype;   //0=disable, 1=highdamp (lowpass), 2=lowdamp (highpass)
        int   idelaylen;
        int   idelayk;
        float lohifb;
        float idelayfb;
        float roomsize;
        float rs;   //rs is used to "normalise" the volume according to the roomsize
        int   comblen[REV_COMBS * 2];
        int   aplen[REV_APS * 2];
        class Unison * bandwidth;

        //Internal Variables
        float *comb[REV_COMBS * 2];
        int    combk[REV_COMBS * 2];
        float  combfb[REV_COMBS * 2]; //feedback-ul fiecarui filtru "comb"
        float  lpcomb[REV_COMBS * 2]; //pentru Filtrul LowPass
        float *ap[REV_APS * 2];
        int    apk[REV_APS * 2];
        float *idelay;
        class AnalogFilter * lpf, *hpf; //filters
};

}

#endif
