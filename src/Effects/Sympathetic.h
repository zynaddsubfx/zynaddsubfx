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
        unsigned char Pvolume=0;       //Volume or E/R
        unsigned char Pdrive=0;        //the input amplification
        unsigned char Plevel=0;        //the output amplification
        unsigned char Ptype=0;         //Distorsion type
        unsigned char Pnegate=0;       //if the input is negated
        unsigned char Plpf=0;          //lowpass filter
        unsigned char Phpf=0;          //highpass filter
        unsigned char Pstereo=0;       //0=mono, 1=stereo
        unsigned char Pq=0;            //0=0.95 ... 127=1.05
        unsigned char Punison_size=0;  //number of unison strings
        unsigned char Punison_frequency_spread=0;
        unsigned char Pstrings=0;
        unsigned char Pbasenote=0;
        unsigned char Pcrossgain=0;
        
        unsigned char spread, spread_old;
        float unison_real_spread_up, unison_real_spread_down;
        
        float freqs[NUM_SYMPATHETIC_STRINGS];
        float baseFreq;
        void setvolume(unsigned char _Pvolume);
        void setlpf(unsigned char _Plpf);
        void sethpf(unsigned char _Phpf);
        void calcFreqs();

        //Real Parameters
        class AnalogFilter * lpfl, *lpfr, *hpfl, *hpfr;
        
        class CombFilterBank * filterBank;
};

}

#endif
