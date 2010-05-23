/*
  ZynAddSubFX - a software synthesizer

  Phaser.cpp - Phaser effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include <cmath>
#include "Phaser.h"
#define PHASER_LFO_SHAPE 2

Phaser::Phaser(const int &insertion_, REALTYPE *efxoutl_, REALTYPE *efxoutr_)
    :Effect(insertion_, efxoutl_, efxoutr_, NULL, 0), old(1), oldgain(0.0), fb(0.0)
{
    setpreset(Ppreset);
    cleanup();
}

Phaser::~Phaser()
{}

/*
 * Effect output
 */
void Phaser::out(const Stereo<float *> &smp)
{
    Stereo<REALTYPE> gain(0.0), lfoVal(0.0);

    lfo.effectlfoout(&lfoVal.l, &lfoVal.r);
    gain.l = (exp(lfoVal.l * PHASER_LFO_SHAPE) - 1) / (exp(PHASER_LFO_SHAPE) - 1.0);
    gain.r = (exp(lfoVal.r * PHASER_LFO_SHAPE) - 1) / (exp(PHASER_LFO_SHAPE) - 1.0);


    gain.l = 1.0 - phase * (1.0 - depth) - (1.0 - phase) * gain.l * depth;
    gain.r = 1.0 - phase * (1.0 - depth) - (1.0 - phase) * gain.r * depth;

    gain.l = limit(gain.l, 0.0f, 1.0f);
    gain.r = limit(gain.r, 0.0f, 1.0f);

    for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
        REALTYPE x   = (REALTYPE) i / SOUND_BUFFER_SIZE;
        REALTYPE x1  = 1.0 - x;
        Stereo<REALTYPE> g(gain.l * x + oldgain.l * x1,
                           gain.l * x + oldgain.r * x1);
        Stereo<REALTYPE> in(smp.l[i] * panning + fb.l,
                            smp.r[i] * (1.0 - panning) + fb.r);

        //Left channel
        for(int j = 0; j < Pstages * 2; j++) { //Phasing routine
            REALTYPE tmp = old.l[j];
            old.l[j] = g.l * tmp + in.l;
            in.l = tmp - g.l *old.l[j];
        }
        //Right channel
        for(int j = 0; j < Pstages * 2; j++) { //Phasing routine
            REALTYPE tmp = old.r[j];
            old.r[j] = g.r * tmp + in.r;
            in.r = tmp - g.r *old.r[j];
        }
        //Left/Right crossing
        Stereo<REALTYPE> tmp = in;
        in.l = tmp.l * (1.0 - lrcross) + tmp.r * lrcross;
        in.r = tmp.r * (1.0 - lrcross) + tmp.l * lrcross;

        fb.l = in.l * feedback;
        fb.r = in.r * feedback;
        efxoutl[i] = in.l;
        efxoutr[i] = in.r;
    }

    oldgain = gain;

    if(Poutsub != 0)
        for(int i = 0; i < SOUND_BUFFER_SIZE; i++) {
            efxoutl[i] *= -1.0;
            efxoutr[i] *= -1.0;
        }
}

/*
 * Cleanup the effect
 */
void Phaser::cleanup()
{
    fb.l     = 0.0;
    fb.r     = 0.0;
    oldgain = Stereo<REALTYPE>(0.0);
    old.l.clear();
    old.r.clear();
}

/*
 * Parameter control
 */
void Phaser::setdepth(unsigned char Pdepth)
{
    this->Pdepth = Pdepth;
    depth = (Pdepth / 127.0);
}


void Phaser::setfb(unsigned char Pfb)
{
    this->Pfb = Pfb;
    feedback = (Pfb - 64.0) / 64.1;
}

void Phaser::setvolume(unsigned char Pvolume)
{
    this->Pvolume = Pvolume;
    outvolume     = Pvolume / 127.0;
    if(insertion == 0)
        volume = 1.0;
    else
        volume = outvolume;
}

void Phaser::setpanning(unsigned char Ppanning)
{
    this->Ppanning = Ppanning;
    panning = Ppanning / 127.0;
}

void Phaser::setlrcross(unsigned char Plrcross)
{
    this->Plrcross = Plrcross;
    lrcross = Plrcross / 127.0;
}

void Phaser::setstages(unsigned char Pstages)
{
    if(Pstages >= MAX_PHASER_STAGES)
        this->Pstages = MAX_PHASER_STAGES - 1;
    else
        this->Pstages = Pstages;
    old = Stereo<Sample>(Pstages * 2);
    cleanup();
}

void Phaser::setphase(unsigned char Pphase)
{
    this->Pphase = Pphase;
    phase = (Pphase / 127.0);
}


void Phaser::setpreset(unsigned char npreset)
{
    const int     PRESET_SIZE = 12;
    const int     NUM_PRESETS = 6;
    unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        //Phaser1
        {64, 64, 36, 0,   0, 64,  110, 64,  1,  0, 0, 20},
        //Phaser2
        {64, 64, 35, 0,   0, 88,  40,  64,  3,  0, 0, 20},
        //Phaser3
        {64, 64, 31, 0,   0, 66,  68,  107, 2,  0, 0, 20},
        //Phaser4
        {39, 64, 22, 0,   0, 66,  67,  10,  5,  0, 1, 20},
        //Phaser5
        {64, 64, 20, 0,   1, 110, 67,  78,  10, 0, 0, 20},
        //Phaser6
        {64, 64, 53, 100, 0, 58,  37,  78,  3,  0, 0, 20}
    };
    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n < PRESET_SIZE; n++)
        changepar(n, presets[npreset][n]);
    Ppreset = npreset;
}


void Phaser::changepar(int npar, unsigned char value)
{
    switch(npar) {
    case 0:
        setvolume(value);
        break;
    case 1:
        setpanning(value);
        break;
    case 2:
        lfo.Pfreq = value;
        lfo.updateparams();
        break;
    case 3:
        lfo.Prandomness = value;
        lfo.updateparams();
        break;
    case 4:
        lfo.PLFOtype = value;
        lfo.updateparams();
        break;
    case 5:
        lfo.Pstereo = value;
        lfo.updateparams();
        break;
    case 6:
        setdepth(value);
        break;
    case 7:
        setfb(value);
        break;
    case 8:
        setstages(value);
        break;
    case 9:
        setlrcross(value);
        break;
    case 10:
        if(value > 1)
            Poutsub = 1;
        else
            Poutsub = value;
        break;
    case 11:
        setphase(value);
        break;
    }
}

unsigned char Phaser::getpar(int npar) const
{
    switch(npar) {
    case 0:
        return Pvolume;
        break;
    case 1:
        return Ppanning;
        break;
    case 2:
        return lfo.Pfreq;
        break;
    case 3:
        return lfo.Prandomness;
        break;
    case 4:
        return lfo.PLFOtype;
        break;
    case 5:
        return lfo.Pstereo;
        break;
    case 6:
        return Pdepth;
        break;
    case 7:
        return Pfb;
        break;
    case 8:
        return Pstages;
        break;
    case 9:
        return Plrcross;
        break;
    case 10:
        return Poutsub;
        break;
    case 11:
        return Pphase;
        break;
    default:
        return 0;
    }
}

