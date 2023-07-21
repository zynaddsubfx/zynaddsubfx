/*
  ZynAddSubFX - a software synthesizer

  Hysteresis.h - Hysteresis Effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef HYSTERESIS_H
#define HYSTERESIS_H

#include "Effect.h"
#include "../Misc/Stereo.h"
#include "JilesAtherton.h"

namespace zyn {

/**Hysteresis Effect*/
class Hysteresis:public Effect
{
    public:
        Hysteresis(EffectParams pars);
        ~Hysteresis();

        void out(const Stereo<float *> &input);
        unsigned char getpresetpar(unsigned char npreset, unsigned int npar);
        void setpreset(unsigned char npreset);
        /**
         * Sets the value of the chosen variable
         *
         * The possible parameters are:
         *   -# Volume
         *   -# Panning
         *   -# Coercitivity -  value of the magnetic field intensity required to return the magnetization of the material to zero
         *   -# Remanence - level of magnetization which remain in the material after the magnetic field intensity is set to zero

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
         *   -# Coercitivity
         *   -# Remanence
         * @param npar number of chosen parameter
         * @return value of parameter
         */
        unsigned char getpar(int npar) const;
        int getnumparams(void);
        void init(void);

        static rtosc::Ports ports;
    private:
        //Parameters
        unsigned char Pvolume;        /**<#1 Volume or Dry/Wetness*/
        unsigned char Pdrive;         /**<#3 remanence of the hysteresis*/
        unsigned char Premanence;     /**<#4 remanence of the hysteresis*/
        unsigned char Pcoercivity;    /**<#5 coercivity of hysteresis loop*/

        void setvolume(unsigned char _Pvolume);
        void setdrive(unsigned char _Pdrive);
        void setremanence(unsigned char _Premanence);
        void setcoercivity(unsigned char _Pcoercivity);
        
        JilesAtherton * ja_l;
        JilesAtherton * ja_r;
};

}

#endif
