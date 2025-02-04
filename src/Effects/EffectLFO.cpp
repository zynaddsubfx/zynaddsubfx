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
#include "globals.h"

namespace zyn {

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
    float output;

    if (Pfreq!=freq ) // calculate coeffs only if cutoff changed
    {
        freq = Pfreq;
        // calculate biquad coefficients
        FcAbs = (freq + 7.0f)*(freq + 7.0f) / 8192; // max value < 40
        K = tan(PI * limit(FcAbs * dt,0.001f,0.4f)); // FcRel * dt_ max 40 * 0.01 = 0.4,
        // LIMIT in case of LFO sampling frequency lower than 100 Hz
        norm = 1.0f / (1.0f + K / 0.7071f + K * K);
        a0 = K * K * norm;
        a1 = 2.0f * a0;
        a2 = a0;
        b1 = 2.0f * (K * K - 1.0f) * norm;
        b2 = (1.0f - K / 0.7071f + K * K) * norm;
    }

    // lp filter the (s&h) random LFO
    output = limit(input * a0 + z1, -1.0f, 1.0f);
    z1 = input * a1 + z2 - b1 * output;
    z2 = input * a2 - b2 * output;
    return output;
}

//Compute the shape of the LFO
float EffectLFO::getlfoshape(float x)
{
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
            //~ if ((x < 0.5) != first_half) {
                first_half = x < 0.5;
                last_random = 2*RND-1;
            //~ }
            return biquad(last_random);


        //when adding more, ensure ::updateparams() gets updated
        default:
            out = cosf(x * 2.0f * PI); //EffectLFO_SINE // TODO: use M_PI ?
    }
    return out;
}

//LFO output
void EffectLFO::effectlfoout(float *outl, float *outr)
{
    float out;

    out = getlfoshape(xl);
    if((lfotype == 0) || (lfotype == 1))
        out *= (ampl1 + xl * (ampl2 - ampl1));
    xl += incx;
    if(xl > 1.0f) {
        xl   -= 1.0f;
        ampl1 = ampl2;
        ampl2 = (1.0f - lfornd) + lfornd * RND;
    }
    *outl = (out + 1.0f) * 0.5f;

    out = getlfoshape(xr);
    if((lfotype == 0) || (lfotype == 1))
        out *= (ampr1 + xr * (ampr2 - ampr1));
    xr += incx;
    if(xr > 1.0f) {
        xr   -= 1.0f;
        ampr1 = ampr2;
        ampr2 = (1.0f - lfornd) + lfornd * RND;
    }
    *outr = (out + 1.0f) * 0.5f;
}

}
