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





namespace zyn {

#define MAX_CHORUS_DELAY 250.0f //ms

// Chorus modes
// CHORUS default chorus mode
// FLANGE flanger mode (very short delays)
// TRIPLE 120° triple phase chorus
// DUAL   180° dual phase chorus

#define CHORUS_MODES \
    CHORUS,\
    FLANGE,\
    TRIPLE,\
    DUAL

/**Chorus and Flange effects*/
class Chorus final:public Effect
{
    public:
        enum ChorusModes {
            CHORUS_MODES
        };

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
        inline float getSample(float* delayline, float mdel, int dk);

        //Chorus Parameters
        unsigned char Pvolume;
        unsigned char Pdepth;      //the depth of the Chorus(ms)
        unsigned char Pdelay;      //the delay (ms)
        unsigned char Pfb;         //feedback
        unsigned char Pflangemode; //mode as described above in CHORUS_MODES
        unsigned char Poutsub;     //if I wish to subtract the output instead of the adding it
        EffectLFO     lfo;         //lfo-ul chorus


        //Parameter Controls
        void setvolume(unsigned char _Pvolume);
        void setdepth(unsigned char _Pdepth);
        void setdelay(unsigned char _Pdelay);
        void setfb(unsigned char _Pfb);

        //Internal Values
        float depth, delay, fb;
        float dlHist, dlNew, lfol;
        float drHist, drNew, lfor;
        float dlHist2, dlNew2;
        float drHist2, drNew2;
        float dlHist3, dlNew3;
        float drHist3, drNew3;
        int   maxdelay;
        Stereo<float *> delaySample;
        int dlk, drk;
        float getdelay(float xlfo);

        float output;
};

}

#endif
