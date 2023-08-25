/*
  ZynAddSubFX - a software synthesizer

  Chorus.h - Chorus and Flange effects
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef CHORUS_H
#define CHORUS_H
#include "Effect.h"
#include "EffectLFO.h"
#include "../Misc/Stereo.h"

#define MAX_CHORUS_DELAY 250.0f //ms

namespace zyn {

/**Chorus and Flange effects*/
class Chorus final:public Effect
{
    public:
        Chorus(EffectParams pars);
        /**Destructor*/
        ~Chorus();
        void out(const Stereo<float *> &input);
        unsigned char getpresetpar(unsigned char npreset, unsigned int npar);
        void setpreset(unsigned char npreset);
        /**
         * Sets the value of the chosen variable
         *
         * The possible parameters are:
         *   -# Volume
         *   -# Panning
         *   -# LFO Frequency
         *   -# LFO Randomness
         *   -# LFO Type
         *   -# LFO stereo
         *   -# Depth
         *   -# Delay
         *   -# Feedback
         *   -# Flange Mode
         *   -# Subtractive
         * @param npar number of chosen parameter
         * @param value the new value
         */
        void changepar(int npar, unsigned char value);
        /**
         * Gets the value of the chosen variable
         *
         * The possible parameters are:
         *   -# Volume
         *   -# Panning
         *   -# LFO Frequency
         *   -# LFO Randomness
         *   -# LFO Type
         *   -# LFO stereo
         *   -# Depth
         *   -# Delay
         *   -# Feedback
         *   -# Flange Mode
         *   -# Subtractive
         * @param npar number of chosen parameter
         * @return the value of the parameter
         */
        unsigned char getpar(int npar) const;
        void cleanup(void);

        static rtosc::Ports ports;
    private:
        //Chorus Parameters
        unsigned char Pvolume;
        unsigned char Pdepth;      //the depth of the Chorus(ms)
        unsigned char Pdelay;      //the delay (ms)
        unsigned char Pfb;         //feedback
        unsigned char Pflangemode; //how the LFO is scaled, to result chorus or flange
        unsigned char Poutsub;     //if I wish to subtract the output instead of the adding it
        EffectLFO     lfo;         //lfo-ul chorus


        //Parameter Controls
        void setvolume(unsigned char _Pvolume);
        void setdepth(unsigned char _Pdepth);
        void setdelay(unsigned char _Pdelay);
        void setfb(unsigned char _Pfb);

        //Internal Values
        float depth, delay, fb;
        float dl1, dl2, dr1, dr2, lfol, lfor;
        int   maxdelay;
        Stereo<float *> delaySample;
        int dlk, drk, dlhi;
        float getdelay(float xlfo);
};

}

#endif
