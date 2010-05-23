/*

  APhaser.cpp  - Approximate digital model of an analog JFET phaser.
  Analog modeling implemented by Ryan Billing aka Transmogrifox.
  November, 2009
  
  Credit to:
  ///////////////////
  ZynAddSubFX - a software synthesizer
 
  Phaser.cpp - Phaser effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  Modified for rakarrack by Josep Andreu
  
  DSP analog modeling theory & practice largely influenced by various CCRMA publications, particularly works by Julius O. Smith.
  ////////////////////
  
  
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

#include <cmath>
#include "APhaser.h"
#include <cstdio>
#include <iostream>
using namespace std;
#define PHASER_LFO_SHAPE 2
#define ONE_  0.99999f        // To prevent LFO ever reaching 1.0 for filter stability purposes
#define ZERO_ 0.00001f        // Same idea as above.

Analog_Phaser::Analog_Phaser(const int & insertion_, REALTYPE *efxoutl_, REALTYPE *efxoutr_)
    :Effect(insertion_, efxoutl_, efxoutr_, NULL, 0), xn1(NULL), yn1(NULL), diff(0.0), oldgain(0.0),
     fb(0.0)
{
    //model mismatch between JFET devices
    offset[0] = -0.2509303f;
    offset[1] = 0.9408924f;
    offset[2] = 0.998f;
    offset[3] = -0.3486182f;
    offset[4] = -0.2762545f;
    offset[5] = -0.5215785f;
    offset[6] = 0.2509303f;
    offset[7] = -0.9408924f;
    offset[8] = -0.998f;
    offset[9] = 0.3486182f;
    offset[10] = 0.2762545f;
    offset[11] = 0.5215785f;

    barber = 0;  //Deactivate barber pole phasing by default

    mis = 1.0f;
    Rmin = 625.0f;// 2N5457 typical on resistance at Vgs = 0
    Rmax = 22000.0f;// Resistor parallel to FET
    Rmx = Rmin/Rmax;
    Rconst = 1.0f + Rmx;  // Handle parallel resistor relationship
    C = 0.00000005f;     // 50 nF
    CFs = (float) 2.0f*(float)SAMPLE_RATE*C;
    invperiod = 1.0f / ((float) SOUND_BUFFER_SIZE);


    Ppreset = 0;
    setpreset(Ppreset);
    cleanup();
}

Analog_Phaser::~Analog_Phaser()
{
    if(xn1.l)
        delete[] xn1.l;
    if(yn1.l)
        delete[] yn1.l;
    if(xn1.r)
        delete[] xn1.r;
    if(yn1.r)
        delete[] yn1.r;
}


/*
 * Effect output
 */
void Analog_Phaser::out(const Stereo<REALTYPE *> &input)
{
    Stereo<REALTYPE> gain(0.0), lfoVal(0.0), mod(0.0), g(0.0), b(0.0), hpf(0.0);

    lfo.effectlfoout(&lfoVal.l, &lfoVal.r);
    mod.l = lfoVal.l*width + depth;
    mod.r = lfoVal.r*width + depth;

    mod.l = limit(mod.l, ZERO_, ONE_);
    mod.r = limit(mod.r, ZERO_, ONE_);

    if(Phyper != 0) {
        //Triangle wave squared is approximately sin on bottom, tri on top
        //Result is exponential sweep more akin to filter in synth with
        //exponential generator circuitry.
        mod.l *= mod.l;
        mod.r *= mod.r;
    }

    //g.l,g.r is Vp - Vgs. Typical FET drain-source resistance follows constant/[1-sqrt(Vp - Vgs)]
    mod.l = sqrtf(1.0f - mod.l);   
    mod.r = sqrtf(1.0f - mod.r);

    diff.r = (mod.r - oldgain.r) * invperiod;
    diff.l = (mod.l - oldgain.l) * invperiod;

    g = oldgain;
    oldgain = mod;

    for (int i = 0; i < SOUND_BUFFER_SIZE; i++)
    {

        g.l += diff.l;// Linear interpolation between LFO samples
        g.r += diff.r;

        Stereo<REALTYPE> xn(input.l[i], input.r[i]);

        if (barber) {
            g.l = fmodf((g.l + 0.25f) , ONE_);
            g.r = fmodf((g.r + 0.25f) , ONE_);
        }

        //Left channel
        for (int j = 0; j < Pstages; j++) {//Phasing routine
            mis = 1.0f + offsetpct*offset[j];
            float d = (1.0f + 2.0f*(0.25f + g.l)*hpf.l*hpf.l*distortion) * mis;  //This is symmetrical. FET is not, so this deviates slightly, however sym dist. is better sounding than a real FET.
            Rconst =  1.0f + mis*Rmx;
            b.l = (Rconst - g.l )/ (d*Rmin);  // This is 1/R. R is being modulated to control filter fc.
            gain.l = (CFs - b.l)/(CFs + b.l);

            yn1.l[j] = gain.l * (xn.l + yn1.l[j]) - xn1.l[j];
            hpf.l = yn1.l[j] + (1.0f-gain.l)*xn1.l[j];  //high pass filter -- Distortion depends on the high-pass part of the AP stage. 

            xn1.l[j] = xn.l;
            xn.l = yn1.l[j];
            if (j==1)
                xn.l += fb.l;  //Insert feedback after first phase stage
        }

        //Right channel
        for (int j = 0; j < Pstages; j++) {//Phasing routine
            mis = 1.0f + offsetpct*offset[j];
            float d = (1.0f + 2.0f*(0.25f + g.r)*hpf.r*hpf.r*distortion) * mis;   // distortion
            Rconst =  1.0f + mis*Rmx;
            b.r = (Rconst - g.r )/ (d*Rmin);
            gain.r = (CFs - b.r)/(CFs + b.r);

            yn1.r[j] = gain.r * (xn.r + yn1.r[j]) - xn1.r[j];
            hpf.r = yn1.r[j] + (1.0f-gain.r)*xn1.r[j];  //high pass filter

            xn1.r[j] = xn.r;
            xn.r = yn1.r[j];
            if (j==1)
                xn.r += fb.r;  //Insert feedback after first phase stage
        }


        fb.l = xn.l * feedback;
        fb.r = xn.r * feedback;
        efxoutl[i] = xn.l;
        efxoutr[i] = xn.r;

    }

    if(Poutsub != 0)
        for(int i = 0; i < SOUND_BUFFER_SIZE; i++)
        {
            efxoutl[i] *= -1.0f;
            efxoutr[i] *= -1.0f;
        }
}

/*
 * Cleanup the effect
 */
void Analog_Phaser::cleanup()
{
    fb = oldgain = Stereo<REALTYPE>(0.0);
    for(int i = 0; i < Pstages; i++)
    {
        xn1.l[i] = 0.0;
        yn1.l[i] = 0.0;
        xn1.r[i] = 0.0;
        yn1.r[i] = 0.0;

    }
}

/*
 * Parameter control
 */
void Analog_Phaser::setwidth(unsigned char Pwidth)
{
    this->Pwidth = Pwidth;
    width = ((float)Pwidth / 127.0f);
}


void Analog_Phaser::setfb(unsigned char Pfb)
{
    this->Pfb = Pfb;
    feedback = (float) (Pfb - 64) / 64.2f;
}

void Analog_Phaser::setvolume(unsigned char Pvolume)
{
    this->Pvolume = Pvolume;
    // outvolume is needed in calling program
    if(insertion == 0) {
        outvolume = pow(0.01, (1.0 - Pvolume / 127.0)) * 4.0;
        volume    = 1.0;
    }
    else
        volume = outvolume = Pvolume / 127.0;
}

void Analog_Phaser::setdistortion(unsigned char Pdistortion)
{
    this->Pdistortion = Pdistortion;
    distortion = (float)Pdistortion / 127.0f;
}

void Analog_Phaser::setoffset(unsigned char Poffset)
{
    this->Poffset = Poffset;  
    offsetpct = (float)Poffset / 127.0f;
}

void Analog_Phaser::setstages(unsigned char Pstages)
{
    if(xn1.l)
        delete[] xn1.l;
    if(yn1.l)
        delete[] yn1.l;
    if(xn1.r)
        delete[] xn1.r;
    if(yn1.r)
        delete[] yn1.r;


    if(Pstages >= MAX_PHASER_STAGES)
        Pstages = MAX_PHASER_STAGES;
    this->Pstages = Pstages;


    xn1 = Stereo<REALTYPE *>(new REALTYPE[Pstages],
                             new REALTYPE[Pstages]);

    yn1 = Stereo<REALTYPE *>(new REALTYPE[Pstages],
                             new REALTYPE[Pstages]);

    cleanup();
}

void Analog_Phaser::setdepth(unsigned char Pdepth)
{
    this->Pdepth = Pdepth;
    depth = (float)(Pdepth - 64) / 127.0f;  //Pdepth input should be 0-127.  depth shall range 0-0.5 since we don't need to shift the full spectrum.
}


void Analog_Phaser::setpreset(unsigned char npreset)
{
    const int PRESET_SIZE = 13;
    const int NUM_PRESETS = 6;
    unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        //Phaser1
        {64, 20, 14, 0, 1, 64, 110, 40, 4, 10, 0, 64, 1},
        //Phaser2
        {64, 20, 14, 5, 1, 64, 110, 40, 6, 10, 0, 70, 1},
        //Phaser3
        {64, 20, 9, 0, 0, 64, 40, 40, 8, 10, 0, 60, 0},
        //Phaser4
        {64, 20, 14, 10, 0, 64, 110, 80, 7, 10, 1, 45, 1},
        //Phaser5
        {25, 20, 240, 10, 0, 64, 25, 16, 8, 100, 0, 25, 0},
        //Phaser6
        {64, 20, 1, 10, 1, 64, 110, 40, 12, 10, 0, 70, 1}
    };
    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n < PRESET_SIZE; n++)
        changepar(n, presets[npreset][n]);
    Ppreset = npreset;
}


void Analog_Phaser::changepar(int npar, unsigned char value)
{
    switch(npar)
    {
        case 0:
            setvolume(value);
            break;
        case 1:
            setdistortion(value);
            break;
        case 2:
            lfo.Pfreq = value;
            lfo.updateparams();
            break;
        case 3:
            lfo.Prandomness = value;
            lfo.updateparams ();
            break;
        case 4:
            lfo.PLFOtype = value;
            lfo.updateparams();
            barber = 0;
            if (value == 2) barber = 1;
            break;
        case 5:
            lfo.Pstereo = value;
            lfo.updateparams();
            break;
        case 6:
            setwidth(value);
            break;
        case 7:
            setfb(value);
            break;
        case 8:
            setstages(value);
            break;
        case 9:
            setoffset(value);
            break;
        case 10:
            if (value > 1)
                value = 1;
            Poutsub = value;
            break;
        case 11:
            setdepth(value);
            break;
        case 12:
            if (value > 1)
                value = 1;
            Phyper = value;
            break;
    }
}

unsigned char Analog_Phaser::getpar(int npar) const
{
    switch(npar)
    {
        case 0:
            return(Pvolume);
            break;
        case 1:
            return(Pdistortion);
            break;
        case 2:
            return(lfo.Pfreq);
            break;
        case 3:
            return(lfo.Prandomness);
            break;
        case 4:
            return(lfo.PLFOtype);
            break;
        case 5:
            return(lfo.Pstereo);
            break;
        case 6:
            return(Pwidth);
            break;
        case 7:
            return(Pfb);
            break;
        case 8:
            return(Pstages);
            break;
        case 9:
            return(Poffset);
            break;
        case 10:
            return(Poutsub);
            break;
        case 11:
            return(Pdepth);
            break;
        case 12:
            return(Phyper);
            break;

        default:
            return(0);
    }
}
