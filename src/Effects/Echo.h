/*
  ZynAddSubFX - a software synthesizer

  Echo.h - Echo Effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef ECHO_H
#define ECHO_H

#include "Effect.h"
#include "../Misc/Stereo.h"

namespace zyn {

/**Echo Effect*/
class Echo final:public Effect
{
    public:
        Echo(EffectParams pars);
        ~Echo();

        void out(const Stereo<float *> &input);
        unsigned char getpresetpar(unsigned char npreset, unsigned int npar);
        void setpreset(unsigned char npreset);
        /**
         * Sets the value of the chosen variable
         *
         * The possible parameters are:
         *   -# Volume
         *   -# Panning
         *   -# Delay
         *   -# L/R Delay
         *   -# L/R Crossover
         *   -# Feedback
         *   -# Dampening
         * @param npar number of chosen parameter
         * @param value the new value
         */
        void changepar(int npar, unsigned char value);

        /**
         * Gets the specified parameter
         *
         * The possible parameters are
         *   -# Volume
         *   -# Panning
         *   -# Delay
         *   -# L/R Delay
         *   -# L/R Crossover
         *   -# Feedback
         *   -# Dampening
         * @param npar number of chosen parameter
         * @return value of parameter
         */
        unsigned char getpar(int npar) const;
        int getnumparams(void);
        void cleanup(void);

        static rtosc::Ports ports;
    private:
        //Parameters
        unsigned char Pvolume;  /**<#1 Volume or Dry/Wetness*/
        unsigned char Pdelay;   /**<#3 Delay of the Echo*/
        unsigned char Plrdelay; /**<#4 L/R delay difference*/
        unsigned char Pfb;      /**<#6Feedback*/
        unsigned char Phidamp;  /**<#7Dampening of the Echo*/

        void setvolume(unsigned char _Pvolume);
        void setdelay(unsigned char _Pdelay);
        void setlrdelay(unsigned char _Plrdelay);
        void setfb(unsigned char _Pfb);
        void sethidamp(unsigned char _Phidamp);

        //Real Parameters
        float fb, hidamp;
        //Left/Right delay lengths
        Stereo<int> delayTime;
        float       lrdelay;
        float       avgDelay;

        void initdelays(void);
        //2 channel ring buffer
        Stereo<float *> delay;
        Stereo<float>   old;

        //position of reading/writing from delaysample
        Stereo<int> pos;
        //step size for delay buffer
        Stereo<int> delta;
        Stereo<int> ndelta;
};

}

#endif
