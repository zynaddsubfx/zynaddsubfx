/*
  ZynAddSubFX - a software synthesizer

  EffectLFO.cpp - Stereo LFO used by some effects
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "EffectLFO.h"
#include "../Misc/Util.h"

#include <cmath>
#include "../globals.h"

namespace zyn {

    constexpr float SAMPLE_RATE_SCALE = 8192.0f;  // 2^13, used for frequency normalization
    constexpr float FREQUENCY_OFFSET = 2.0f;      // Frequency offset to prevent division by zero
    constexpr float MAX_FC_NORMALIZED = 40.0f;    // Maximum normalized cutoff frequency

    // Resonance and Q factor constants
    constexpr float DEFAULT_RESONANCE = 0.7071f;  // Butterworth response (Q = 1/sqrt(2))

    // Stability limits for K calculation
    constexpr float MIN_NORMALIZED_FREQ = 0.001f;
    constexpr float MAX_NORMALIZED_FREQ = 0.4f;


EffectLFO::EffectLFO(float srate_f, float bufsize_f)
    :Pfreq(40),
      Prandomness(0),
      PLFOtype(0),
      Pstereo(64),
      xl(0.0f),
      xr(0.0f),
      ampl1(RND),
      ampl2(RND),
      ampr1(RND),
      ampr2(RND),
      lfornd(0.0f),
      samplerate_f(srate_f),
      buffersize_f(bufsize_f),
      dt(bufsize_f/srate_f)
{
    updateparams();
}

EffectLFO::~EffectLFO() {}

//Update the changed parameters
void EffectLFO::updateparams(void)
{
    float lfofreq = (powf(2.0f, Pfreq / 127.0f * 10.0f) - 1.0f) * 0.03f;
    incx = fabsf(lfofreq) * buffersize_f / samplerate_f;
    if(incx > 0.49999999f)
        incx = 0.499999999f;  //Limit the Frequency

    lfornd = Prandomness / 127.0f;
    lfornd = (lfornd > 1.0f) ? 1.0f : lfornd;

    if(PLFOtype > 2)
        PLFOtype = 0;  //this has to be updated if more lfo's are added
    lfotype = PLFOtype;
    xr      = xl + (Pstereo - 64.0f) / 127.0f + 1.0f;
    xr      -= floorf(xr);
}

float EffectLFO::biquad(float input)
{

    if (Pfreq!=freq ) // calculate coeffs only if cutoff changed
    {
        freq = (float)Pfreq;
        // Normalize cutoff frequency to prevent extreme values
        // FcAbs represents the normalized angular frequency
        const float normalizedCutoff = (freq + FREQUENCY_OFFSET) * (freq + FREQUENCY_OFFSET) / SAMPLE_RATE_SCALE;

        // Apply frequency warping using bilinear transform
        // Limit the normalized frequency to ensure numerical stability
        const float normalizedFreq = limit(normalizedCutoff * dt, MIN_NORMALIZED_FREQ, MAX_NORMALIZED_FREQ);
        const float K = tan(PI * normalizedFreq);  // Frequency warping for digital domain

        // Calculate filter coefficients for normalized low-pass filter
        const float norm = 1.0f / (1.0f + K / DEFAULT_RESONANCE + K * K);

        // Numerator coefficients (feedforward path)
        a0 = K * K * norm;
        a1 = 2.0f * a0;
        a2 = a0;

        // Denominator coefficients (feedback path)
        b1 = 2.0f * (K * K - 1.0f) * norm;
        b2 = (1.0f - K / DEFAULT_RESONANCE + K * K) * norm;
    }

    // lp filter the (s&h) random LFO
    const float output = limit(input * a0 + z1, -1.0f, 1.0f);
    z1 = input * a1 + z2 - b1 * output;
    z2 = input * a2 - b2 * output;
    return output;
}

//Compute the shape of the LFO
float EffectLFO::getlfoshape(float x)
{
    x = fmodf(x, 1.0f);
    float out;
    switch(lfotype) {
        case 1: //EffectLFO_TRIANGLE
            if((x > 0.0f) && (x < 0.25f))
                out = 4.0f * x;
            else
            if((x > 0.25f) && (x < 0.75f))
                out = 2.0f - 4.0f * x;
            else
                out = 4.0f * x - 4.0f;
            break;
        case 2: //EffectLFO_NOISE
            float rnd_val;
            rnd_val = 8.0f*RND-4.0f; // range -4 ... +4
            return biquad(rnd_val);


        //when adding more, ensure ::updateparams() gets updated
        default:
            out = cosf(x * 2.0f * PI); //EffectLFO_SINE
    }
    return out;
}

//LFO output
void EffectLFO::effectlfoout(float *outl, float *outr, float phaseOffset)
{
    float out;
    // left stereo signal
    out = getlfoshape(xl+phaseOffset);
    if((lfotype == 0) || (lfotype == 1))
        out *= (ampl1 + xl * (ampl2 - ampl1));
    *outl = (out + 1.0f) * 0.5f;
    // update left phase for master lfo
    if(phaseOffset==0.0f) {
        xl += incx;
        if(xl > 1.0f) {
            xl   -= 1.0f;
            ampl1 = ampl2;
            ampl2 = (1.0f - lfornd) + lfornd * RND;
        }
    }

    // right stereo signal
    out = getlfoshape(xr+phaseOffset);
    if((lfotype == 0) || (lfotype == 1))
        out *= (ampr1 + xr * (ampr2 - ampr1));
    *outr = (out + 1.0f) * 0.5f;

    // update right phase for master lfo
    if(phaseOffset==0.0f) {
        xr += incx;
        if(xr > 1.0f) {
            xr   -= 1.0f;
            ampr1 = ampr2;
            ampr2 = (1.0f - lfornd) + lfornd * RND;
        }
    }

}

}
