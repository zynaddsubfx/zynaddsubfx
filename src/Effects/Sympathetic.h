/*
  ZynAddSubFX - a software synthesizer

  Sympathetic.cpp - Distorsion effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef SYMPATHETIC_H
#define SYMPATHETIC_H

#include "Effect.h"

namespace zyn {

class Sympathetic:public Effect
{
    
    public:
    Sympathetic(EffectParams pars);
    ~Sympathetic();
        void out(const Stereo<float *> &smp);
        unsigned char getpresetpar(unsigned char npreset, unsigned int npar);
        void setpreset(unsigned char npreset);
        void changepar(int npar, unsigned char value);
        unsigned char getpar(int npar) const;
        void cleanup(void);
        void applyfilters(float *efxoutl, float *efxoutr);

        static rtosc::Ports ports;
    private:
        //Parameters
        unsigned char Pvolume;       //Volume or E/R
        unsigned char Pdrive;        //the input amplification
        unsigned char Plevel;        //the output amplification
        unsigned char Ptype;         //Distorsion type
        unsigned char Pnegate;       //if the input is negated
        unsigned char Plpf;          //lowpass filter
        unsigned char Phpf;          //highpass filter
        unsigned char Pstereo;       //0=mono, 1=stereo
        unsigned char Pq;            //0=0.95 ... 127=1.05
        unsigned char Punison_size;  //number of unison strings
        unsigned char Punison_frequency_spread;
        
        unsigned char spread, spread_old;
        float unison_real_spread_up, unison_real_spread_down;
        
        float freqs[NUM_SYMPATHETIC_STRINGS];

        void setvolume(unsigned char _Pvolume);
        void setlpf(unsigned char _Plpf);
        void sethpf(unsigned char _Phpf);
        void calcFreqs(unsigned char unison_size, unsigned char spread);

        //Real Parameters
        class AnalogFilter * lpfl, *lpfr, *hpfl, *hpfr;
        
        class CombFilterBank * filterBank;
};

}

#endif
