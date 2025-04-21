/*
  ZynAddSubFX - a software synthesizer

  Reverse.h - Reverse Delay Effect
  Copyright (C) 2023-2024 Michael Kirchner
  Author: Michael Kirchner

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef REVERSE_H
#define REVERSE_H

#include "Effect.h"

namespace zyn {

class Reverter;

/**Reverse Effect*/
class Reverse final:public Effect
{
    public:
        Reverse(EffectParams pars, const AbsTime *time_=nullptr);
        ~Reverse();
        void update();
        void out(const Stereo<float *> &input);
        unsigned char getpresetpar(unsigned char npreset, unsigned int npar);
        void setpreset(unsigned char npreset);
        /**
         * Sets the value of the chosen variable
         *
         * The possible parameters are:
         *   -# Volume
         *   -# Panning (member of parent class)
         *   -# Delay
         *   -# Stereo
         *   -# Phase
         *   -# Crossfade
         *   -# Sync Mode

         * @param npar number of chosen parameter
         * @param value the new value
         */
        void changepar(int npar, unsigned char value);

        /**
         * Gets the specified parameter
         *
         * The possible parameters are:
         *   -# Volume
         *   -# Panning (member of parent class)
         *   -# Delay
         *   -# Stereo
         *   -# Phase
         *   -# Crossfade
         *   -# Sync Mode


         * @param npar number of chosen parameter
         * @return value of parameter
         */
        unsigned char getpar(int npar) const;
        int getnumparams(void);
        void cleanup(void);

        static rtosc::Ports ports;
    private:
        //Parameters
        unsigned char Pvolume;     /**< #0 Volume or Dry/Wetness */
        unsigned char Pdelay;      /**< #2 Length of reversed segment, 127 = 1.5s */
        unsigned char Pphase;      /**< #3 Phase offset for delay effect */
        unsigned char Pcrossfade;  /**< #4 Crossfade duration between segments */
        unsigned char PsyncMode;   /**< #5 Synchronization mode setting */
        unsigned char Pstereo;     /**< #6 Stereo mode flag */

        void setvolume(unsigned char _Pvolume);
        void setdelay(unsigned char _Pdelay);
        void setphase(unsigned char _Pphase);
        void setcrossfade(unsigned char value);
        void setsyncMode(unsigned char value);

        const AbsTime *time;

        Reverter* reverterL;
        Reverter* reverterR;

        float tick;                 // number of ticks passed in the current measure
                                    // - used to predict the position of the next beat
        unsigned int currentSubbufferIndex = 0;
        float tick_hist;
        float tick_at_host_buffer_start;
        bool playing_hist;
};

}

#endif
