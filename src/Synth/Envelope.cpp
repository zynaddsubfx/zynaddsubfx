/*
  ZynAddSubFX - a software synthesizer

  Envelope.cpp - Envelope implementation
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include "Envelope.h"
#include "../Params/EnvelopeParams.h"

namespace zyn {

Envelope::Envelope(EnvelopeParams &pars, float basefreq, float bufferdt,
        WatchManager *m, const char *watch_prefix)
    :watchOut(m, watch_prefix, "out")
{
    envpoints = pars.Penvpoints;
    if(envpoints > MAX_ENVELOPE_POINTS)
        envpoints = MAX_ENVELOPE_POINTS;
    envsustain     = (pars.Penvsustain == 0) ? -1 : pars.Penvsustain;
    forcedrelease   = pars.Pforcedrelease;
    envstretch     = powf(440.0f / basefreq, pars.Penvstretch / 64.0f);
    linearenvelope = pars.Plinearenvelope;
    repeating = pars.Prepeating;

    if(!pars.Pfreemode)
        pars.converttofree();

    mode = pars.Envmode;

    //for amplitude envelopes
    if((mode == 1) && !linearenvelope)
        mode = 2;                              //change to log envelope
    if((mode == 2) && linearenvelope)
        mode = 1;                              //change to linear

    for(int i = 0; i < MAX_ENVELOPE_POINTS; ++i) {
        const float dtstretched = pars.getdt(i) * envstretch;
        if(dtstretched > bufferdt)
            envdt[i] = bufferdt / dtstretched;
        else
            envdt[i] = 2.0f;  //any value larger than 1

        envcpy[i] = pars.envcpy[i];
        switch(mode) {
            case ADSR_dB:
                envval[i] = (1.0f - pars.Penvval[i] / 127.0f) * -40;

                break;
            case ASR_freqlfo:
                envval[i] =
                    (powf(2, 6.0f
                          * fabsf(pars.Penvval[i]
                                 - 64.0f) / 64.0f) - 1.0f) * 100.0f;
                if(pars.Penvval[i] < 64){
                    envval[i] = -envval[i];
                    envcpy[i] = -envcpy[i];
                }

                break;
            case ADSR_filter:
                envval[i] = (pars.Penvval[i] - 64.0f) / 64.0f * 6.0f; //6 octaves (filtru)
                break;
            case ASR_bw:
                envval[i] = (pars.Penvval[i] - 64.0f) / 64.0f * 10;
                break;
            case ADSR_lin:
            default:
                envval[i] = pars.Penvval[i] / 127.0f;
                break;
        }
    }

    inct      = envdt[1];
}

Envelope::~Envelope()
{}


/*
 * Release the key (note envelope)
 */
void Envelope::releasekey()
{
    if(keyreleased)
        return;
    keyreleased = true;
    if(forcedrelease) {
        tRelease = t;
        t = 0.0f;

    }
}

void Envelope::forceFinish(void)
{
    envfinish = true;
}

void Envelope::watch(float time, float value)
{
    float pos[2];
    float factor1;
    float factor2;
    pos[0] = time;
    switch(mode) {
        case ADSR_dB:
            pos[1] = 1 - value / -40.f;
            watchOut(pos, 2);
            break;
        case ASR_freqlfo:
            factor1 = log(value/100. + 1.) / (6. * log(2));
            factor2 = log(1. - value/100.) / (6. * log(2));
            pos[1] = ((0.5 * factor1) >= 0) ? (0.5 * factor1 + 0.5) : (0.5  - factor2 * 0.5);
            watchOut(pos, 2);
            break;
        case ADSR_filter:
            pos[1] = (value + 6.) / 12.f;
            watchOut(pos, 2);
            break;
        case ASR_bw:
            pos[1] = (value + 10.) / 20.f;
            watchOut(pos, 2);
            break;
        case ADSR_lin:
        default:
            pos[1] = value;
            watchOut(pos, 2);
            break;
    }
}

/**
 * @brief Cubic Bezier interpolation with backward compatibility to linear interpolation
 *
 * This function implements a cubic Bezier curve interpolation that automatically
 * defaults to linear interpolation when offset parameters are zero. The control
 * points are calculated relative to the linear path between start and end points.
 *
 * @param a     Start value (point P0)
 * @param bOffs Relative offset factor for the first control point (P1)
 *             - Value of 0.0 places P1 at 1/3 of the linear path
 *             - Positive values move P1 beyond the linear path toward end point
 *             - Negative values move P1 before the linear path toward start point
 *             - Typical range: [-0.3, 0.3] for reasonable curves
 *
 * @param cOffs Relative offset factor for the second control point (P2)
 *             - Value of 0.0 places P2 at 2/3 of the linear path
 *             - Positive values move P2 beyond the linear path toward end point
 *             - Negative values move P2 before the linear path toward start point
 *             - Typical range: [-0.3, 0.3] for reasonable curves
 *
 * @param d     End value (point P3)
 * @param t     Interpolation parameter in range [0, 1]
 *             - 0.0 returns start value (a)
 *             - 1.0 returns end value (d)
 *             - Values outside [0, 1] are automatically clamped
 *
 * @return float Interpolated value at parameter t
 *
 * @note The function is backward compatible: when bOffs = 0 and cOffs = 0,
 *       the interpolation is perfectly linear between a and d.
 *
 * @note For a = d (zero difference), the function returns a constant value
 *       regardless of offset parameters, which is mathematically correct.
 */
inline float bezier3(float a, float bOffs, float cOffs, float d, float t)
{
    // Clamp parameter t to valid range [0, 1] for numerical stability
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);

    // Calculate total difference between end and start points
    const float diff = d - a;

    // Calculate Bezier control points relative to linear path
    // Base positions for linear interpolation (1/3 and 2/3 along the line)
    const float b = a + diff * (0.3333333333f + bOffs);
    const float c = a + diff * (0.6666666666f + cOffs);

    // Precompute powers of t and (1-t) for Bernstein polynomials
    const float mt = 1.0f - t;
    const float t2 = t * t;
    const float mt2 = mt * mt;

    // Cubic Bezier formula using Bernstein basis:
    // B(t) = (1-t)³·P0 + 3·(1-t)²·t·P1 + 3·(1-t)·t²·P2 + t³·P3
    return mt * mt2 * a +          // (1-t)³ term for start point P0
           3.0f * mt2 * t * b +    // 3·(1-t)²·t term for control point P1
           3.0f * mt * t2 * c +    // 3·(1-t)·t² term for control point P2
           t * t2 * d;             // t³ term for end point P3
}

/*
 * Envelope Output
 */
float Envelope::envout(bool doWatch)
{
    float out;
    if(envfinish) { //if the envelope is finished
        envoutval = envval[envpoints - 1];
        if(doWatch) {
            watch(envpoints - 1, envoutval);
        }
        return envoutval;
    }

    if((currentpoint == envsustain + 1) && !keyreleased) { //if it is sustaining now
        envoutval = envval[envsustain];
        bool zerorelease = true;
        for (auto i = envsustain; i<envpoints; i++)
            if (envval[i] != -40.0f) zerorelease = false;
        if (zerorelease &&                             //if sustaining at zero with zero until env ends
            (mode == ADSR_lin || mode == ADSR_dB)) {   // and its an amp envelope
            envfinish = true;   // finish voice to free resources
        }
        if(doWatch) {
            watch(envsustain, envoutval);
        }
        return envoutval;
    }

    if(keyreleased && forcedrelease) { //do the forced release
        int releaseindex = (envsustain < 0) ? (envpoints - 1) : (envsustain + 1); //if there is no sustain point, use the last point for release

        if(envdt[releaseindex] < 0.00000001f)
            out = envval[releaseindex];
        else
            out = bezier3(envoutval, envcpy[releaseindex*2-1], envcpy[releaseindex*2], envval[releaseindex], t);

        t += envdt[releaseindex];

        if(t >= 1.0f) { // move to the next segment
            currentpoint = envsustain + 2;
            forcedrelease = 0;
            t    = 0.0f;
            inct = envdt[currentpoint];
            if((currentpoint >= envpoints) || (envsustain < 0))
                envfinish = true;
        }

        if(doWatch) {
            watch(releaseindex + t, envoutval);
        }

        return out;
    }


    // --- NEW LOGIC: Pre-calculation of envelope position ---

    // 1. Advance time and handle segment transitions
    float new_t = t + inct;
    int new_currentpoint = currentpoint;

    bool segment_changed = false;

    // Handle cases where we cross one or more segment boundaries
    while(new_t >= 1.0f && !envfinish) {
        new_t -= 1.0f;  // Carry remainder to next segment
        segment_changed = true;

        // Check if we've reached the last point
        if(new_currentpoint >= envpoints - 1) {
            envfinish = true;
            break;
        }
        // Check if we should stop at sustain point (with repeat mode)
        else if (repeating && new_currentpoint == envsustain && !keyreleased) {
            // Stay at sustain point in repeat mode
            new_t = 0.0f;  // Reset to beginning of sustain
            break;
        }
        else {
            // Move to next segment
            new_currentpoint++;
        }

        // Update increment for new segment
        inct = envdt[new_currentpoint];
    }

    // 2. Calculate output based on current (not new!) position
    if(t >= 1.0f) {
        // We're exactly at a control point
        out = envval[currentpoint];
    } else {
        // Interpolate between current and next point
        out = bezier3(envval[currentpoint - 1],
                     envcpy[currentpoint*2-1],
                     envcpy[currentpoint*2],
                     envval[currentpoint],
                     t);
    }

    // 3. Update envelope state for next call
    if(segment_changed && !envfinish) {
        currentpoint = new_currentpoint;
        t = new_t;
    } else {
        t = new_t;
    }
    envoutval = out;

    if(doWatch) {
        watch(currentpoint + t + tRelease, envoutval);
    }
    return out;
}

/*
 * Envelope Output (dB)
 */
float Envelope::envout_dB()
{
    float out;
    if(linearenvelope)
        return envout(true);

    if((currentpoint == 1) && (!keyreleased || !forcedrelease)) { //first point is always lineary interpolated <- seems to have odd effects
        const float v1 = EnvelopeParams::env_dB2rap(envval[0]);
        const float v2 = EnvelopeParams::env_dB2rap(envval[1]);
        out = v1 + (v2 - v1) * t;

        t += inct;

        if(t >= 1.0f) {
            t    = 0.0f;
            inct = envdt[2];
            currentpoint++;
            out = v2;
        }

        if(out > 0.001f)
            envoutval = EnvelopeParams::env_rap2dB(out);
        else
            envoutval = MIN_ENVELOPE_DB;
        out = envoutval;
    } else
        out = envout(false);

    watch(currentpoint + t, out);
    return EnvelopeParams::env_dB2rap(out);

}

bool Envelope::finished() const
{
    return envfinish;
}

}
