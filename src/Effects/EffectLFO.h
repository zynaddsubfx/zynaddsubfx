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

/**
 * LFO (Low Frequency Oscillator) for Effect objects
 *
 * Generates low-frequency control signals for audio effects with support for:
 * - Multiple waveform types
 * - Stereo phase offset
 * - Frequency randomness/variation
 * - Noise generation with filtering
 *
 * \todo Evaluate if this should inherit from a base LFO class
 */
class EffectLFO
{
public:
    /**
     * Constructs an EffectLFO object.
     *
     * @param srate_f Sample rate in Hz.
     * @param bufsize_f Buffer size in samples.
     */
    EffectLFO(float srate_f, float bufsize_f);

    /**
     * Destructs the EffectLFO object.
     */
    ~EffectLFO();

    /**
     * Calculates the output of the effect LFO for two signals (stereo)
     *
     * When phaseOffset is 0: updates internal phase and returns current LFO values
     * When phaseOffset != 0: returns phase-shifted output without updating internal state
     *
     * @param outl Pointer to store left output channel LFO value (-1.0 to 1.0)
     * @param outr Pointer to store right output channel LFO value (-1.0 to 1.0)
     * @param phaseOffset Phase offset in radians or normalized units (0.0 = no offset)
     *                   Non-zero values return phase-shifted output without state update
     */
    void effectlfoout(float *outl, float *outr, float phaseOffset = 0.0f);

    /**
     * Updates the LFO parameters based on the current settings.
     *
     * Recalculates internal parameters such as:
     * - Frequency increment
     * - Randomness factors
     * - Filter coefficients
     * - Amplitude modulation parameters
     *
     * Should be called when any LFO parameter (Pfreq, Prandomness, etc.) changes
     */
    void updateparams(void);

    // LFO control parameters (typically 0-127 MIDI range unless specified)
    unsigned char Pfreq;        //!< Frequency parameter (0-127), controls LFO speed
    unsigned char Prandomness;  //!< Randomness parameter (0-127), adds variation to frequency
    unsigned char PLFOtype;     //!< LFO type parameter (0-2): 0=Sine, 1=Triangle, 2=Square
    unsigned char Pstereo;      //!< Stereo phase spread parameter (-64 to +64), controls stereo width

private:
    /**
     * Calculates the LFO waveform shape based on the current phase and type.
     *
     * @param x Normalized phase value (0.0 to 1.0)
     * @return The calculated LFO shape value in range [-1.0, 1.0]
     */
    float getlfoshape(float x);

    /**
     * Processes a sample through the biquad low-pass filter.
     *
     * Used for smoothing the noise LFO output to reduce high-frequency content.
     * Implements the difference equation:
     * y[n] = a0*x[n] + a1*x[n-1] + a2*x[n-2] - b1*y[n-1] - b2*y[n-2]
     *
     * @param input Input sample to filter
     * @return Filtered output sample
     */
    float biquad(float input);

    // Phase management
    float xl;       //!< Phase accumulator for left channel (0.0 to 1.0)
    float xr;       //!< Phase accumulator for right channel (0.0 to 1.0)
    float incx;     //!< Phase increment per sample, determined by frequency

    // Amplitude modulation and randomness
    float ampl1, ampl2; //!< Left channel amplitude modulation factors
    float ampr1, ampr2; //!< Right channel amplitude modulation factors
    float lfornd;       //!< Current randomness factor for frequency variation

    // LFO configuration
    char lfotype;       //!< Current LFO type (0=Sine, 1=Triangle, 2=Square)

    // System configuration
    float samplerate_f; //!< Sample rate in Hz
    float buffersize_f; //!< Buffer size in samples
    float dt;           //!< Buffer length in seconds (buffersize_f / samplerate_f)
    float freq;         //!< Cutoff frequency for noise oscillator low-pass filter

    // Biquad filter coefficients and state for noise LFO smoothing
    float a0; //!< Feedforward coefficient for current sample
    float a1; //!< Feedforward coefficient for n-1 sample
    float a2; //!< Feedforward coefficient for n-2 sample
    float b1; //!< Feedback coefficient for n-1 sample
    float b2; //!< Feedback coefficient for n-2 sample
    float z1; //!< Filter state variable (delay line for n-1)
    float z2; //!< Filter state variable (delay line for n-2)
};

} // namespace

#endif
