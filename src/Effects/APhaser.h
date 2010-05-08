/*
  ZynAddSubFX - a software synthesizer
 
  Phaser.h - Phaser effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  Modified for rakarrack by Josep Andreu

  Further modified for rakarrack by Ryan Billing (Transmogrifox) to model Analog Phaser behavior 2009
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License 
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#ifndef APHASER_H
#define APHASER_H
#include "../globals.h"
#include "Effect.h"
#include "EffectLFO.h"

#define MAX_PHASER_STAGES 12

class Analog_Phaser:public Effect
{
    public:
        Analog_Phaser(const int & insertion_, REALTYPE * efxoutl_, REALTYPE * efxoutr_);
        ~Analog_Phaser();
        void out(const Stereo<REALTYPE *> &input);
        void setpreset(unsigned char npreset);
        void changepar(int npar, unsigned char value);
        unsigned char getpar(int npar) const;
        void cleanup();
        //unsigned char Ppreset;

    private:
        //Phaser parameters
        EffectLFO lfo;              //Phaser modulator
        unsigned char Pvolume;      //Used in Process.cpp to set wet/dry mix
        unsigned char Pdistortion;  //Model distortion added by FET element
        unsigned char Pwidth;       //Phaser width (LFO amplitude)
        unsigned char Pfb;          //feedback
        unsigned char Poffset;      //Model mismatch between variable resistors
        unsigned char Pstages;      //Number of first-order All-Pass stages
        unsigned char Poutsub;      //if I wish to subtract the output instead of the adding it
        unsigned char Phyper;       //lfo^2 -- converts tri into hyper-sine
        unsigned char Pdepth;       //Depth of phaser sweep
        unsigned char Pbarber;      //Enable parber pole phasing


        //Control parameters
        void setvolume(unsigned char Pvolume);
        void setdistortion(unsigned char Pdistortion);
        void setwidth(unsigned char Pwidth);
        void setfb(unsigned char Pfb);
        void setoffset(unsigned char Poffset);
        void setstages(unsigned char Pstages);
        void setdepth(unsigned char Pdepth);

        //Internal Variables
        bool barber; //Barber pole phasing flag
        REALTYPE distortion, fb, width, offsetpct, fbl, fbr, depth;
        REALTYPE *lxn1, *lyn1,*rxn1, *ryn1, *offset;
        REALTYPE oldlgain, oldrgain, rdiff, ldiff, invperiod;

        float mis;
        float Rmin;     // 2N5457 typical on resistance at Vgs = 0
        float Rmax;     // Resistor parallel to FET
        float Rmx;      // Rmin/Rmax to avoid division in loop
        float Rconst;   // Handle parallel resistor relationship
        float C;        // Capacitor
        float CFs;      // A constant derived from capacitor and resistor relationships
};

#endif
