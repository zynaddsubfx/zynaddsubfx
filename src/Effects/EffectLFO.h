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

#include "../globals.h"

namespace zyn {

/**LFO for some of the Effect objects
 * \todo see if this should inherit LFO*/
class EffectLFO
{
    public:
     /**
     * Constructs an EffectLFO object.
     *
     * @param srate_f Sample rate.
     * @param bufsize_f Buffer size.
     * @param time Pointer to an AbsTime object (optional).
     */
    EffectLFO(float srate_f, float bufsize_f, const AbsTime *time = nullptr);

    /**
     * Destructs the EffectLFO object.
     */
    ~EffectLFO();

    /**
     * Processes the LFO and outputs the result to the provided pointers.
     *
     * @param outl Pointer to the left output channel.
     * @param outr Pointer to the right output channel.
     */
    void effectlfoout(float *outl, float *outr, float phaseOffset = 0);

    /**
     * Updates the LFO parameters based on the current settings.
     */
    void updateparams(void);

    unsigned char Pfreq;  //!< Frequency parameter (0-127)
    unsigned char Prandomness;  //!< Randomness parameter (0-127)
    unsigned char PLFOtype;  //!< LFO type parameter (0-2)
    unsigned char Pstereo;  //!< Stereo parameter (-64 to 64)

private:
    /**
     * Calculates the LFO shape based on the current phase and type.
     *
     * @param x Phase value (0-1).
     * @return The calculated LFO shape value.
     */
    float getlfoshape(float x);

    float xl, xr;  //!< Phase accumulators for left and right channels
    float incx;  //!< Increment for phase accumulators
    float ampl1, ampl2, ampr1, ampr2;  //!< Amplitude modulation parameters
    float lfornd;  //!< Randomness factor
    char lfotype;  //!< Current LFO type

    // Current setup
    float samplerate_f;  //!< Sample rate
    float buffersize_f;  //!< Buffer size

    const AbsTime *time;  //!< Pointer to an AbsTime object
    unsigned int numerator;  //!< The numerator of the time signature to calculate the frequency from system tempo(bpm)
                            //!< If it is 0 Pfreq is used.
    unsigned int denominator;  //!< The denominator of the time signature to calculate the frequency from system tempo(bpm)
};

}

#endif
