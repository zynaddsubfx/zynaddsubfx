/*
  ZynAddSubFX - a software synthesizer

  Phaser.h - Phaser effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Copyright (C) 2009-2010 Ryan Billing
  Copyright (C) 2010-2010 Mark McCurry
  Author: Nasca Octavian Paul
          Ryan Billing
          Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef PHASER_H
#define PHASER_H
#include "../globals.h"
#include "Effect.h"
#include "EffectLFO.h"

#define MAX_PHASER_STAGES 12

namespace zyn {

class Phaser final:public Effect
{
    public:
        Phaser(EffectParams pars);
        ~Phaser();
        void out(const Stereo<float *> &input);
        unsigned char getpresetpar(unsigned char npreset, unsigned int npar);
        void setpreset(unsigned char npreset);
        void changepar(int npar, unsigned char value);
        unsigned char getpar(int npar) const;
        void cleanup();

        static rtosc::Ports ports;
    private:
        //Phaser parameters
        EffectLFO     lfo;          //Phaser modulator
        unsigned char Pvolume;      //Used to set wet/dry mix
        unsigned char Pdistortion;  //Model distortion added by FET element
        unsigned char Pdepth;       //Depth of phaser sweep
        unsigned char Pwidth;       //Phaser width (LFO amplitude)
        unsigned char Pfb;          //feedback
        unsigned char Poffset;      //Model mismatch between variable resistors
        unsigned char Pstages;      //Number of first-order All-Pass stages
        unsigned char Poutsub;      //if I wish to subtract the output instead of adding
        unsigned char Pphase;
        unsigned char Phyper;       //lfo^2 -- converts tri into hyper-sine
        unsigned char Panalog;

        //Control parameters
        void setvolume(unsigned char Pvolume);
        void setdepth(unsigned char Pdepth);
        void setfb(unsigned char Pfb);
        void setdistortion(unsigned char Pdistortion);
        void setwidth(unsigned char Pwidth);
        void setoffset(unsigned char Poffset);
        void setstages(unsigned char Pstages);
        void setphase(unsigned char Pphase);

        //Internal Variables
        bool  barber; //Barber pole phasing flag
        float distortion, width, offsetpct;
        float feedback, depth, phase;
        Stereo<float *> old, xn1, yn1;
        Stereo<float>   diff, oldgain, fb;
        float invperiod;
        float offset[12];

        float mis;
        float Rmin;     // 3N5457 typical on resistance at Vgs = 0
        float Rmax;     // Resistor parallel to FET
        float Rmx;      // Rmin/Rmax to avoid division in loop
        float Rconst;   // Handle parallel resistor relationship
        float C;        // Capacitor
        float CFs;      // A constant derived from capacitor and resistor relationships

        void analog_setup();
        void AnalogPhase(const Stereo<float *> &input);
        //analog case
        float applyPhase(float x, float g, float fb,
                         float &hpf, float *yn1, float *xn1);

        void normalPhase(const Stereo<float *> &input);
        float applyPhase(float x, float g, float *old);
};

}

#endif
