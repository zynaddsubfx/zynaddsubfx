/*
  ZynAddSubFX - a software synthesizer

  EffectLFO.h - Stereo LFO used by some effects
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef EFFECT_LFO_H
#define EFFECT_LFO_H

namespace zyn {

/**LFO for some of the Effect objects
 * \todo see if this should inherit LFO*/
class EffectLFO
{
    public:
        EffectLFO(float srate_f, float bufsize_f);
        ~EffectLFO();
        // if phaseOffset!=0 we don't want to update the phase but get a phased shifted output of the last one.
        /**
         * calculate the output of the effect LFO for two signals (stereo)
         * @param outl pointer to write the result for left side to
         * @param outr pointer to write the result for right side to
         * @param phaseOffset phaseoffset 
         * if phaseOffset is not 0 we don't want to update the phase 
         * but get the phased shifted output of the last one.
         * */
        void effectlfoout(float *outl, float *outr, float phaseOffset = 0.0f);
        void updateparams(void);
        unsigned char Pfreq;
        unsigned char Prandomness;
        unsigned char PLFOtype;
        unsigned char Pstereo; // 64 is centered
    private:
        float getlfoshape(float x);

        float xl, xr;
        float incx;
        float ampl1, ampl2, ampr1, ampr2; //necessary for "randomness"
        float lfornd;
        char  lfotype;

        // current setup
        float samplerate_f;
        float buffersize_f;
};

}

#endif
