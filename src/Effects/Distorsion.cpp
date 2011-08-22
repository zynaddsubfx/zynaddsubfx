/*
  ZynAddSubFX - a software synthesizer

  Distorsion.cpp - Distorsion effect
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

#include "Distorsion.h"
#include "../DSP/AnalogFilter.h"
#include "../Misc/WaveShapeSmps.h"
#include <cmath>

Distorsion::Distorsion(const int &insertion_,
                       float *efxoutl_,
                       float *efxoutr_)
    :Effect(insertion_, efxoutl_, efxoutr_, NULL, 0)
{
    lpfl = new AnalogFilter(2, 22000, 1, 0);
    lpfr = new AnalogFilter(2, 22000, 1, 0);
    hpfl = new AnalogFilter(3, 20, 1, 0);
    hpfr = new AnalogFilter(3, 20, 1, 0);


    //default values
    Pvolume = 50;
    Pdrive        = 90;
    Plevel        = 64;
    Ptype         = 0;
    Pnegate       = 0;
    Plpf          = 127;
    Phpf          = 0;
    Pstereo       = 0;
    Pprefiltering = 0;

    setpreset(Ppreset);
    cleanup();
}

Distorsion::~Distorsion()
{
    delete lpfl;
    delete lpfr;
    delete hpfl;
    delete hpfr;
}

/*
 * Cleanup the effect
 */
void Distorsion::cleanup()
{
    lpfl->cleanup();
    hpfl->cleanup();
    lpfr->cleanup();
    hpfr->cleanup();
}


/*
 * Apply the filters
 */

void Distorsion::applyfilters(float *efxoutl, float *efxoutr)
{
    lpfl->filterout(efxoutl);
    hpfl->filterout(efxoutl);
    if(Pstereo != 0) { //stereo
        lpfr->filterout(efxoutr);
        hpfr->filterout(efxoutr);
    }
}


/*
 * Effect output
 */
void Distorsion::out(const Stereo<float *> &smp)
{
    int      i;
    float l, r, lout, rout;

    float inputvol = pow(5.0, (Pdrive - 32.0) / 127.0);
    if(Pnegate != 0)
        inputvol *= -1.0;

    if(Pstereo != 0) { //Stereo
        for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
            efxoutl[i] = smp.l[i] * inputvol * pangainL;
            efxoutr[i] = smp.r[i] * inputvol * pangainR;
        }
    }
    else {
        for(i = 0; i < SOUND_BUFFER_SIZE; i++)
            efxoutl[i] = (smp.l[i] * pangainL + smp.r[i] * pangainR) * inputvol;
    }

    if(Pprefiltering != 0)
        applyfilters(efxoutl, efxoutr);

    //no optimised, yet (no look table)
    waveShapeSmps(SOUND_BUFFER_SIZE, efxoutl, Ptype + 1, Pdrive);
    if(Pstereo != 0)
        waveShapeSmps(SOUND_BUFFER_SIZE, efxoutr, Ptype + 1, Pdrive);

    if(Pprefiltering == 0)
        applyfilters(efxoutl, efxoutr);

    if(Pstereo == 0)
        for(i = 0; i < SOUND_BUFFER_SIZE; i++)
            efxoutr[i] = efxoutl[i];

    float level = dB2rap(60.0 * Plevel / 127.0 - 40.0);
    for(i = 0; i < SOUND_BUFFER_SIZE; i++) {
        lout = efxoutl[i];
        rout = efxoutr[i];
        l    = lout * (1.0 - lrcross) + rout * lrcross;
        r    = rout * (1.0 - lrcross) + lout * lrcross;
        lout = l;
        rout = r;

        efxoutl[i] = lout * 2.0 * level;
        efxoutr[i] = rout * 2.0 * level;
    }
}


/*
 * Parameter control
 */
void Distorsion::setvolume(unsigned char Pvolume)
{
    this->Pvolume = Pvolume;

    if(insertion == 0) {
        outvolume = pow(0.01, (1.0 - Pvolume / 127.0)) * 4.0;
        volume    = 1.0;
    }
    else
        volume = outvolume = Pvolume / 127.0;
    ;
    if(Pvolume == 0)
        cleanup();
}

void Distorsion::setlpf(unsigned char Plpf)
{
    this->Plpf = Plpf;
    float fr = exp(pow(Plpf / 127.0, 0.5) * log(25000.0)) + 40;
    lpfl->setfreq(fr);
    lpfr->setfreq(fr);
}

void Distorsion::sethpf(unsigned char Phpf)
{
    this->Phpf = Phpf;
    float fr = exp(pow(Phpf / 127.0, 0.5) * log(25000.0)) + 20.0;
    hpfl->setfreq(fr);
    hpfr->setfreq(fr);
}


void Distorsion::setpreset(unsigned char npreset)
{
    const int     PRESET_SIZE = 11;
    const int     NUM_PRESETS = 6;
    unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        //Overdrive 1
        {127, 64,  35,  56, 70, 0, 0, 96,  0,   0,   0  },
        //Overdrive 2
        {127, 64,  35,  29, 75, 1, 0, 127, 0,   0,   0  },
        //A. Exciter 1
        {64,  64,  35,  75, 80, 5, 0, 127, 105, 1,   0  },
        //A. Exciter 2
        {64,  64,  35,  85, 62, 1, 0, 127, 118, 1,   0  },
        //Guitar Amp
        {127, 64,  35,  63, 75, 2, 0, 55,  0,   0,   0  },
        //Quantisize
        {127, 64,  35,  88, 75, 4, 0, 127, 0,   1,   0  }
    };


    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n < PRESET_SIZE; n++)
        changepar(n, presets[npreset][n]);
    if(insertion == 0)
        changepar(0, (int) (presets[npreset][0] / 1.5));           //lower the volume if this is system effect
    Ppreset = npreset;
    cleanup();
}


void Distorsion::changepar(int npar, unsigned char value)
{
    switch(npar) {
        case 0:
            setvolume(value);
            break;
        case 1:
            setpanning(value);
            break;
        case 2:
            setlrcross(value);
            break;
        case 3:
            Pdrive = value;
            break;
        case 4:
            Plevel = value;
            break;
        case 5:
            if(value > 13)
                Ptype = 13; //this must be increased if more distorsion types are added
            else
                Ptype = value;
            break;
        case 6:
            if(value > 1)
                Pnegate = 1;
            else
                Pnegate = value;
            break;
        case 7:
            setlpf(value);
            break;
        case 8:
            sethpf(value);
            break;
        case 9:
            Pstereo = (value > 1) ? 1 : value;
            break;
        case 10:
            Pprefiltering = value;
            break;
    }
}

unsigned char Distorsion::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return Plrcross;
        case 3:  return Pdrive;
        case 4:  return Plevel;
        case 5:  return Ptype;
        case 6:  return Pnegate;
        case 7:  return Plpf;
        case 8:  return Phpf;
        case 9:  return Pstereo;
        case 10: return Pprefiltering;
        default: return 0;
    }
}

