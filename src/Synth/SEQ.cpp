/*
  ZynAddSubFX - a software synthesizer

  SEQ.cpp - SEQ implementation
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "SEQ.h"
#include "../Params/SEQParams.h"
#include "../Misc/Util.h"

#include <cstdlib>
#include <cstdio>
#include <cmath>

namespace zyn {

SEQ::SEQ(const SEQParams &seqpars_, const AbsTime &t, WatchManager *m,
        const char *watch_prefix)
    :time(t),
    delayTime(t, seqpars_.delay), // 0..4 sec Idea: delay time as 1/4 beats
    dt(t.dt()),
    seqpars(seqpars_),
    watchOut(m, watch_prefix, "out")
{

    //max 2x/octave
    if(seqpars.numerator==0) {

        if (seqpars.freq < 0.001)
            duration = 1000.0f;
        else
            duration = 1.0/seqpars.freq;

    } else {
        duration = 240.0f / limit(((float(time.tempo)) * seqpars.denominator / seqpars.numerator), 0.001f, 10000.0f);
    }

    if (seqpars.continous)
        tRef = 0;
    else
        tRef = time.time();

    z1 = 0.0;
    z2 = 0.0;
}

SEQ::~SEQ()
{}

float SEQ::baseOut()
{
    float fraction = (float)(time.time() - tRef) * time.dt() / duration;
    int index = (int)fraction;
    //~ printf("seqpars.steps: %d\n", seqpars.steps);
    int currentSegment = index % limit((int)seqpars.steps, 1, 128);
    //~ printf("index: %d, currentSegment: %d\n", index, currentSegment);
    float seq_out = seqpars.sequence[currentSegment];
    //~ printf("seq_out: %f\n", seq_out);
    return biquad(seq_out);
}


float SEQ::biquad(float input)
{
    float output;
    if (seqpars.cutoff!=cutoff ) // calculate coeffs only if cutoff changed
    {
        cutoff = seqpars.cutoff;
        if (cutoff!= MAX_CUTOFF) // at cutoff 0.0f we bypass filter, no coeffs needed
        {
            // calculate biquad coefficients
            FcAbs = limit(cutoff, 1.0f, 40.0f);
            K = tan(PI * limit(FcAbs * dt,0.001f,0.4f)); // FcRel * dt max 40 * 0.01 = 0.4,
            // LIMIT in case of SEQ sampling frequency lower than 100 Hz

            norm = 1.0f / (1.0f + K / 0.7071f + K * K);
            a0 = K * K * norm;
            a1 = 2.0f * a0;
            a2 = a0;
            b1 = 2.0f * (K * K - 1.0f) * norm;
            b2 = (1.0f - K / 0.7071f + K * K) * norm;
        }
    }
    if (cutoff != MAX_CUTOFF) // at cutoff 127 we bypass filter, nothing to do
    {
        output = limit(input * a0 + z1, -1.0f, 1.0f);
        z1 = input * a1 + z2 - b1 * output;
        z2 = input * a2 - b2 * output;
    }
    return (cutoff==MAX_CUTOFF) ? input : output; // at cutoff 127 bypass filter
}


float SEQ::seqout()
{
    //update internals XXX TODO cleanup
    if ( ! seqpars.time || seqpars.last_update_timestamp == seqpars.time->time())
    {
        if((seqpars.numerator==0)) {
            if (seqpars.freq < 0.001) {
                //~ printf("z109 seqpars.freq = %f\n", seqpars.freq);
                duration = 1000.0f;
            }
            duration = 1.0/seqpars.freq;
            //~ printf("z113 seqpars.freq = %f\n", seqpars.freq);
        } else {
            duration = 240.0f / limit(((float(time.tempo)) * seqpars.denominator / seqpars.numerator), 0.001f, 10000.0f);
        }
    }

    float out = baseOut();

    switch(seqpars.fel) {
        case consumer_location_type_t::amp:
            seqintensity = seqpars.intensity;
            break;
        case consumer_location_type_t::filter:
            seqintensity = seqpars.intensity * 4.0f;
            break; //in octave
        case consumer_location_type_t::freq:
            seqintensity = 2047.0f; //in centi
            break;
        case consumer_location_type_t::unspecified:
            seqintensity = 2047.0f; //powf(2, seqpars.intensity / 127.0f * 11.0f) - 1.0f; //in centi
            break;
        }

    out *= seqintensity;

    if(delayTime.inFuture())
    {
        return 0.0f;
    }

    return out;
}

/*
 * SEQ out (for amplitude)
 */
float SEQ::ampseqout()
{
    return limit(1.0f - seqintensity + seqout(), -1.0f, 1.0f);
}

}
