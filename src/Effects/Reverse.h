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

#ifndef REVERSE_H
#define REVERSE_H

#include "Effect.h"
#include "../Misc/Stereo.h"
#include "../DSP/CombFilter.h"

namespace zyn {

/**Reverse Effect*/
class Reverse final:public Effect
{
    public:
        Reverse(EffectParams pars);
        ~Reverse();

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
        unsigned char Pstereo; // 64 is centered

        void setvolume(unsigned char _Pvolume);
        void setdelay(unsigned char _Pdelay);

        CombFilter* combfilterL;
        CombFilter* combfilterR;
};

}

#endif