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

// coefficients for calculating gainbwd from Pq
// gainbwd = gainbwd_offset + Pq * gainbwd_factor
// designed for gainbwd range up to 1.0 at Pq==127
const float gainbwd_offset = 0.873f;
const float gainbwd_factor = 0.001f;

// number of piano keys with single string
const unsigned int num_single_strings = 12;
// number of piano keys with triple strings
const unsigned int num_triple_strings = 48;

// frequencies of a guitar in standard e tuning
const float guitar_freqs[6] = {82.4f, 110.0f, 146.8f, 196.0f, 246.9f, 329.6f};

class Sympathetic final:public Effect
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
        unsigned char Pstrings;      //number of strings
        unsigned char Pbasenote;     //midi note of lowest string

        unsigned char spread, spread_old;

        float freqs[NUM_SYMPATHETIC_STRINGS];
        float baseFreq;

        void setvolume(unsigned char _Pvolume);
        void setlpf(unsigned char _Plpf);
        void sethpf(unsigned char _Phpf);
        void calcFreqs();
        void calcFreqsGeneric();
        void calcFreqsPiano();
        void calcFreqsGuitar();

        //Real Parameters
        class AnalogFilter * lpfl, *lpfr, *hpfl, *hpfr;

        class CombFilterBank * filterBank;
};

}

#endif
