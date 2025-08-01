/*
  ZynAddSubFX - a software synthesizer

  WaveShapeSmps.cpp - Sample Distortion
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "WaveShapeSmps.h"
#include <cmath>
#include <cassert>

namespace zyn {

// Constants
constexpr float MAX_DRIVE = 127.0f;
constexpr float OFFSET_CENTER = 64.0f;
constexpr float OFFSET_SCALE = 64.0f;
constexpr float FUNC_PAR_SCALE = 127.0f;

/**
 * @brief Computes a polynomial residual value for smoothing discontinuities (PolyBLAMP).
 *
 * @param smp Sample value
 * @param ws  Threshold window size
 * @param dMax Maximum transition width
 * @return Residual correction to reduce aliasing
 */
float polyblampres(float smp, float ws, float dMax)
{
    // Formula from: Esqueda, Välimäki, Bilbao (2015): ALIASING REDUCTION IN SOFT-CLIPPING ALGORITHMS
    // http://dafx16.vutbr.cz/dafxpapers/18-DAFx-16_paper_33-PN.pdf pg 123, table 1
    // Four-point polyBLAMP residual:
    // [−2T, T] d^5/120
    // [−T, 0] −d^5/40 + d^4/24 + d^3/12 + d^2/12 + d/24 + 1/120
    // [0, T] d^5/40 − d^4/12 + d^2/3 − d/2 + 7/30
    // [T, 2T] −d^5/120 + d^4/24 − d^3/12 + d^2/12 − d/24 + 1/120
    if(dMax == 0.0f) return 0.0f;

    float dist = fabsf(smp) - ws;
    float res = 0.0f, d1, d2, d3, d4, d5;

    if(fabsf(dist) < dMax) {
        if(dist < -dMax / 2.0f) {
            d1 = (dist + dMax) / dMax * 2.0f;// [-dMax ... -dMax/2] -> [0 ... 1]
            res = d1 * d1 * d1 * d1 * d1 / 120.0f;
        } else if(dist < 0.0f) {
            d1 = (dist + dMax / 2.0f) / dMax * 2.0f;// [-dMax/2 ... 0] -> [0 ... 1]
            d2 = d1 * d1;
            d3 = d2 * d1;
            d4 = d3 * d1;
            d5 = d4 * d1;
            res = (-d5 / 40.0f) + (d4 / 24.0f) + (d3 / 12.0f) + (d2 / 12.0f) + (d1 / 24.0f) + (1.0f / 120.0f);
        } else if(dist < dMax / 2.0f) {
            d1 = dist / dMax * 2.0f;//[0 ... dMax/2] -> [0 ... 1]
            d2 = d1 * d1;
            d4 = d2 * d2;
            d5 = d4 * d1;
            res = (d5 / 40.0f) - (d4 / 12.0f) + (d2 / 3.0f) - (d1 / 2.0f) + (7.0f / 30.0f);
        } else {
            d1 = (dist - dMax / 2.0f) / dMax * 2.0f;//[dMax/2 ... dMax] -> [0 ... 1]

            d2 = d1 * d1;
            d3 = d2 * d1;
            d4 = d3 * d1;
            d5 = d4 * d1;
            res = (-d5 / 120.0f) + (d4 / 24.0f) - (d3 / 12.0f) + (d2 / 12.0f) - (d1 / 24.0f) + (1.0f / 120.0f);
        }
    }
    return res * dMax / 2.0f;
}

// f(x) = x / ((1+|x|^n)^(1/n)) // tanh approximation for n=2.5
// Formula from: Yeh, Abel, Smith (2007): SIMPLIFIED, PHYSICALLY-INFORMED MODELS OF DISTORTION AND OVERDRIVE GUITAR EFFECTS PEDALS
static float YehAbelSmith(float x, float exp)
{
    return x / (powf((1+powf(fabsf(x),exp)),(1/exp)));
}

/**
 * @brief Apply arctangent waveshaping for soft saturation.
 *        Non-linear distortion that increases with input amplitude.
 *
 * @param n     Number of samples
 * @param smps  Pointer to sample buffer
 * @param ws    Waveshaping "drive" amount
 * @param offs  DC offset added before processing
 */
void processArctangent(int n, float* smps, float ws, float offs) {
    ws = powf(10, ws * ws * 3.0f) - 1.0f + 0.001f; // Compute ws, scale factor for the waveshaper
    float offsetCompensation = atanf(offs * ws) / atanf(ws); // Calculate offset compensation
    for(int i = 0; i < n; ++i) {
        smps[i] += offs; // Add offset to the sample
        smps[i] *= ws; // Apply scaling
        smps[i] = atanf(smps[i]) / atanf(ws); // Apply arctangent waveshaping
        smps[i] -= offsetCompensation; // Subtract offset compensation
    }
}

/**
 * @brief Apply asymmetric sine-based waveshaping.
 *        Produces asymmetric distortion dependent on amplitude.
 *
 * @param n     Number of samples
 * @param smps  Pointer to samples
 * @param ws    Waveshaping drive
 * @param offs  DC offset for asymmetry control
 */
void processAsymmetric(int n, float* smps, float ws, float offs) {
    ws = ws * ws * 32.0f + 0.0001f;
    float tmpv = (ws < 1.0f) ? sinf(ws) + 0.1f : 1.1f;
    for(int i = 0; i < n; ++i)
        smps[i] = (sinf((smps[i] + offs) * (1.1f + ws - ws * (smps[i] + offs))) - sinf(offs * (0.1f + ws - ws * offs))) / tmpv;
}

/**
 * @brief Apply cubic polynomial-based distortion with soft clipping.
 *
 * @param n     Number of samples
 * @param smps  Sample buffer
 * @param ws    Waveshaping strength
 * @param offs  Offset to control symmetry
 */
void processPow(int n, float* smps, float ws, float offs) {
    ws = ws * ws * ws * 20.0f + 0.0001f; //Pow
    for(int i = 0; i < n; ++i) {
        float x = (smps[i] + offs) * ws;
        float xo = offs * ws;
        float v = (fabsf(x) < 1.0f) ? (x - x * x * x) * 3.0f : 0.0f;
        float vo = (fabsf(xo) < 1.0f) ? (xo - xo * xo * xo) * 3.0f : 0.0f;
        if(ws < 1.0f) {
            v /= ws;
            vo /= ws;
        }
        smps[i] = v - vo;
    }
}

/**
 * @brief Apply sine function-based distortion for harmonic folding effects.
 *
 * @param n     Number of samples
 * @param smps  Sample buffer
 * @param ws    Frequency scaling
 * @param offs  Offset (center shift)
 */
void processSine(int n, float* smps, float ws) {
    ws = ws * ws * ws * 32.0f + 0.0001f; //Sine
    float tmpv = (ws < 1.57f) ? sinf(ws) : 1.0f;
    for(int i = 0; i < n; ++i)
        smps[i] = sinf(smps[i] * ws) / tmpv;
}

/**
 * @brief Apply sample quantization (bit crushing-style distortion).
 *
 * @param n     Sample count
 * @param smps  Input/output buffer
 * @param ws    Step size of quantization
 */
void processQuantize(int n, float* smps, float ws) {
    ws = ws * ws + 0.000001f; //Quantisize
    for(int i = 0; i < n; ++i)
        smps[i] = floor(smps[i] / ws + 0.5f) * ws;
}

/**
 * @brief Apply modulated triangle-wave folding (asin of sin input).
 *
 * @param n     Number of samples
 * @param smps  Signal buffer
 * @param ws    Modulation frequency
 * @param offs  Input signal bias (offset)
 */
void processZigzag(int n, float* smps, float ws) {
    ws = ws * ws * ws * 32.0f + 0.0001f; //Zigzag
    float tmpv = (ws < 1.0f) ? sinf(ws) : 1.0f;
    for(int i = 0; i < n; ++i)
        smps[i] = asinf(sinf(smps[i] * ws)) / tmpv;
}

/**
 * @brief Soft class A-style limiter with anti-aliasing using PolyBLAMP.
 *
 * @param n     Sample count
 * @param smps  Sample buffer
 * @param ws    Limiting threshold
 * @param par   PolyBLAMP smoothing width
 * @param offs  DC offset applied before limiting
 */
void processLimiter(int n, float* smps, float ws, float par, float offs) {
    ws = powf(2.0f, -ws * ws * 8.0f); // Compute ws, scale factor for the waveshaper
    par = par / 4; // Scale parameter
    if(par > ws - 0.01f) par = ws - 0.01f; // Ensure par is within bounds

    // Precalculate offset compensation with distortion function
    float resOffset = polyblampres(offs, ws, par);
    float offsetCompensation = offs >= 0 ?
                ( offs >= ws ? ws - resOffset : offs - resOffset ) :
                ( offs <= -ws ? -ws + resOffset : offs + resOffset );

    for(int i = 0; i < n; ++i) {
        smps[i] += offs; // Add offset to the sample
        float res = polyblampres(smps[i], ws, par); // Apply limiter distortion function
        if (smps[i] >= 0)
            smps[i] = (smps[i] > ws ? ws - res : smps[i] - res); // Positive sample handling
        else
            smps[i] = (smps[i] < -ws ? -ws + res : smps[i] + res); // Negative sample handling
        smps[i] -= offsetCompensation; // Subtract offset compensation
        smps[i] /= ws; // Apply inverse scaling
    }
}

/**
 * @brief Clip signal above a threshold using PolyBLAMP.
 *        Lower half of the waveform remains unaffected.
 *
 * @param n     Number of samples
 * @param smps  Sample buffer
 * @param ws    Upper clipping threshold
 * @param par   Anti-aliasing smooth factor (PolyBLAMP)
 * @param offs  Offset shift before processing
 */
void processUpperLimiter(int n, float* smps, float ws, float par, float offs) {
    ws = powf(2.0f, -ws * ws * 8.0f); //Upper Limiter
    if (par > ws - 0.01f) par = ws - 0.01f;
    for(int i = 0; i < n; ++i) {
        smps[i] += offs;
        float res = polyblampres(smps[i], ws, par);
        if (smps[i] > ws)
            smps[i] = ws - res;
        else
            smps[i] = smps[i] - res;
        // remove offset
        float res_offs = polyblampres(offs, ws, par);
        if (offs > ws)
            smps[i] -= ws - res_offs;
        else
            smps[i] -= offs - res_offs;
        smps[i] /= ws;
    }
}

/**
 * @brief Clip signal below a threshold using PolyBLAMP.
 *        Upper half stays clean.
 *
 * @param n     Number of samples
 * @param smps  Sample buffer
 * @param ws    Lower clipping threshold
 * @param par   PolyBLAMP smoothing width
 * @param offs  Offset before clipping
 */
void processLowerLimiter(int n, float* smps, float ws, float par, float offs) {
    ws = powf(2.0f, -ws * ws * 8.0f); //Lower Limiter
    if(par > ws - 0.01f) par = ws - 0.01f;
    for(int i = 0; i < n; ++i) {
        smps[i] += offs;
        float res = polyblampres(smps[i], ws, par);
        if (smps[i] < -ws)
            smps[i] = -ws + res;
        else
            smps[i] = smps[i] + res;
        float res_offs = polyblampres(offs, ws, par);
        if (offs < -ws)
            smps[i] -= -ws + res_offs;
        else
            smps[i] -= offs + res_offs;
        smps[i] /= ws;
    }
}

/**
 * @brief Inverted limiter for diode-like distortion curves (gap at zero).
 *        Uses PolyBLAMP to soften edge transitions.
 *
 * @param n     Sample count
 * @param smps  Sample buffer
 * @param ws    Window size for clipping
 * @param par   PolyBLAMP smoothing factor
 * @param offs  Offset to shift clipping range
 */
void processInverseLimiter(int n, float* smps, float ws, float par, float offs) {
    ws = (powf(2.0f, ws * 6.0f) - 1.0f) / powf(2.0f, 6.0f); // Compute ws, scale factor for the waveshaper
    if (par > ws - 0.01f) par = ws - 0.01f; // Ensure par is within bounds

            // Precalculate offset compensation with distortion function
    float resOffset = polyblampres(offs, ws, par);
    float offsetCompensation = offs >= 0 ?
            (offs > ws ? offs - ws + resOffset : resOffset) :
            (offs < -ws ? offs + ws - resOffset : -resOffset);

    for(int i = 0; i < n; ++i) {
        smps[i] += offs; // Add offset to the sample
        float res = polyblampres(smps[i], ws, par); // Compute polyblamp edge smoothing function
        if(smps[i] >= 0)
            smps[i] = (smps[i] > ws ? smps[i] - ws + res : res); // Positive sample handling
        else
            smps[i] = (smps[i] < -ws ? smps[i] + ws - res : -res); // Negative sample handling
        smps[i] -= offsetCompensation; // Subtract offset compensation
    }
}

/**
 * @brief Distortion via signal wrapping (modulo arithmetic).
 *        Adds aliasing intentionally; PolyBLAMP can reduce artifacts.
 *
 * @param n     Number of samples
 * @param smps  Input buffer
 * @param ws    Wrap scaling factor
 * @param par   PolyBLAMP filter width
 * @param offs  Offset bias
 */
void processClip(int n, float* smps, float ws, float par, float offs) {
    ws = powf(5.0f, ws * ws * 1.0f) - 1.0f; //Clip
    if(par < 0.0001f) par = 0.0001f; // Verhindere zu kleine Werte
    for(int i = 0; i < n; ++i) {
        smps[i] += offs;
        float x = smps[i] * (ws + 0.5f) * 0.9999f;
        float clipped = x - floor(0.5f + x);
        // PolyBLAMP an den Rändern anwenden
        float frac = x - floor(x);
        if(frac < par)
            clipped -= polyblampres(frac, 0.0f, par);
        else if(1.0f - frac < par)
            clipped += polyblampres(1.0f - frac, 0.0f, par);
        smps[i] = clipped;
        // Offset rückgängig machen
        float x_offs = offs * (ws + 0.5f) * 0.9999f;
        float clipped_offs = x_offs - floor(0.5f + x_offs);
        smps[i] -= clipped_offs;
    }
}

/**
 * @brief Complex asymmetric polynomial distortion with offset bias.
 *
 * @param n     Sample count
 * @param smps  Signal buffer
 * @param ws    Nonlinearity thickness
 * @param offs  Offset shift
 */
void processAsym2(int n, float* smps, float ws, float offs) {
    ws = ws * ws * ws * 30.0f + 0.001f; //Asym2
    float tmpv = (ws < 0.3f) ? ws : 1.0f;
    for(int i = 0; i < n; ++i) {
        float tmp = (smps[i] + offs) * ws;
        float tmpo = offs * ws;
        float v = ((tmp > -2.0f) && (tmp < 1.0f)) ? tmp * (1.0f - tmp) * (tmp + 2.0f) : 0.0f;
        float vo = ((tmpo > -2.0f) && (tmpo < 1.0f)) ? tmpo * (1.0f - tmpo) * (tmpo + 2.0f) : 0.0f;
        smps[i] = (v - vo) / tmpv;
    }
}

/**
 * @brief Exponential asymmetric distortion that “snaps” outside limits.
 *
 * @param n     Sample buffer size
 * @param smps  Audio data
 * @param ws    Scaling factor for shaping
 * @param offs  Input offset
 */
void processPow2(int n, float* smps, float ws, float offs) {
    ws = ws * ws * ws * 32.0f + 0.0001f; //Pow2
    float tmpv = (ws < 1.0f) ? ws * (1.0f + ws) / 2.0f : 1.0f;
    for(int i = 0; i < n; ++i) {
        float x = (smps[i] + offs) * ws;
        float xo = offs * ws;
        float v = ((x > -1.0f) && (x < 1.618034f)) ? x * (1.0f - x) : ((x > 0.0f) ? -1.0f : -2.0f);
        float vo = ((xo > -1.0f) && (xo < 1.618034f)) ? xo * (1.0f - xo) : ((xo > 0.0f) ? -1.0f : -2.0f);
        smps[i] = (v - vo) / tmpv;
    }
}

/**
 * @brief Map signal through sigmoid curve from -1 to 1, centered by offset.
 *        Offers controllable dynamic compression.
 *
 * @param n     Sample count
 * @param smps  Input/output array
 * @param ws    Controls slope of activation
 * @param offs  Pre-bias of signal
 */
void processSigmoid(int n, float* smps, float ws, float offs) {
    ws = powf(ws, 5.0f) * 80.0f + 0.0001f; // Compute ws, scale factor for the waveshaper
    // Calculate tmpv for normalization
    float tmpv = (ws > 10.0f) ? 0.5f : 0.5f - 1.0f / (expf(ws) + 1.0f);
    for(int i = 0; i < n; ++i) {
        smps[i] += offs;
        float tmp = smps[i] * ws;
        if(tmp < -10.0f) tmp = -10.0f;
        else if(tmp > 10.0f) tmp = 10.0f;
        tmp = 0.5f - 1.0f / (expf(tmp) + 1.0f);

        float tmpo = offs * ws;
        if(tmpo < -10.0f) tmpo = -10.0f;
        else if(tmpo > 10.0f) tmpo = 10.0f;
        tmpo = 0.5f - 1.0f / (expf(tmpo) + 1.0f); // Apply sigmoid waveshaping


        smps[i] = tmp / tmpv; //Normalize
        smps[i] -= tmpo / tmpv; // Subtract offset compensation
    }
}

/**
 * @brief Parametrized tanh-based soft limiter with adjustable knee.
 *
 * @param n     Number of samples
 * @param smps  Signal buffer
 * @param ws    Input gain
 * @param par   Curvature factor
 * @param offs  DC offset
 *
 * f(x) = x / ((1+|x|^n)^(1/n)) // tanh approximation for n=2.5
 * Formula from: Yeh, Abel, Smith (2007): SIMPLIFIED, PHYSICALLY-INFORMED MODELS OF DISTORTION AND OVERDRIVE GUITAR EFFECTS PEDALS
 */
void processTanhLimiter(int n, float* smps, float ws, float par, float offs) {
    par = (20.0f) * par * par + (0.1f) * par + 1.0f; //Pfunpar=32 -> n=2.5
    ws = powf(10, ws * ws * 2.0f) - 1.0f + 0.1f; // Compute ws, scale factor for the waveshaper
    // precalc offset with distortion function applied
    float offsetCompensation = YehAbelSmith(offs, par);
    for(int i = 0; i < n; ++i) {
        smps[i] *= ws;      // multiply signal to drive it in the saturation of the function
        smps[i] += offs;    // add dc offset
        smps[i] = YehAbelSmith(smps[i], par);
        smps[i] -= offsetCompensation;
    }
}

/**
 * @brief Apply waveshaping based on a 3rd-order polynomial approximation of tanh.
 *        Saturates softly near ±1
 *
 * @param n     Sample count
 * @param smps  Samples
 * @param ws    Drive amount
 * @param offs  Offset before shaping
 *
 * f(x) = 1.5 * (x-(x^3/3))
 * Formula from: https://ccrma.stanford.edu/~jos/pasp/Soft_Clipping.html
 * modified with factor 1.5 to go through [1,1] and [-1,-1]
 */
void processCubic(int n, float* smps, float ws, float offs) {
    //~ ws = ws * ws * ws * 20.0f + 0.168f; // plain cubic at drive=44
    ws = powf(10, ws * ws * 1.5f) - 1.0f + 0.1f; // Compute ws, scale factor for the waveshaper
    // precalc offset with distortion function applied
    float offsetCompensation = 1.5 * (offs - (offs*offs*offs / 3.0));

    for(int i = 0; i < n; ++i) {
        smps[i] *= ws; // multiply signal to drive it in the saturation of the function
        smps[i] += offs; // add dc offset
        if(fabsf(smps[i]) < 1.0f)
            smps[i] = 1.5f * (smps[i] - (smps[i] * smps[i] * smps[i] / 3.0f));
        else
            smps[i] = (smps[i] > 0 ? 1.0f : -1.0f);
        //subtract offset with distortion function applied
        smps[i] -= offsetCompensation;
    }
}

/**
 * @brief Apply parabolic soft clipping for signals within [-1, 1],
 *
 *
 * @param n     Number of samples
 * @param smps  Buffers
 * @param ws    Drive control
 * @param offs  Offset control
 *
 * f(x) = x*(2-abs(x))
 * Formula of cubic changed to square but still going through [1,1] and [-1,-1]
 */
void processSquare(int n, float* smps, float ws, float offs) {
    // Square distortion waveshaper
    //~ ws = ws * ws * ws * 20.0f + 0.168f; // plain square at drive=44
    ws = powf(10, ws * ws * 1.5f) - 1.0f + 0.1f; // Compute ws, scale factor for the waveshaper
    // precalc offset with distortion function applied
    float offsetCompensation = offs*(2-fabsf(offs));
    for(int i = 0; i < n; ++i) {
        smps[i] *= ws; // multiply signal to drive it in the saturation of the function
        smps[i] += offs; // add dc offset
        if(fabsf(smps[i]) < 1.0f)
            smps[i] = smps[i] * (2.0f - fabsf(smps[i]));
        else
            smps[i] = (smps[i] > 0 ? 1.0f : -1.0f);
        //subtract offset with distortion function applied
        smps[i] -= offsetCompensation;
    }
}

// Main processing function
void waveShapeSmps(int n, float* smps, unsigned char type, unsigned char drive, unsigned char offset, unsigned char funcpar)
{
    assert(smps != nullptr && n > 0);

    float ws = static_cast<float>(drive) / MAX_DRIVE;
    float par = static_cast<float>(funcpar) / FUNC_PAR_SCALE;
    float offs = (static_cast<float>(offset) - OFFSET_CENTER) / OFFSET_SCALE;

    switch (type) {
        case 1:  processArctangent(n, smps, ws, offs); break;
        case 2:  processAsymmetric(n, smps, ws, offs); break;
        case 3:  processPow(n, smps, ws, offs); break;
        case 4:  processSine(n, smps, ws); break;
        case 5:  processQuantize(n, smps, ws); break;
        case 6:  processZigzag(n, smps, ws); break;
        case 7:  processLimiter(n, smps, ws, par, offs); break;
        case 8:  processUpperLimiter(n, smps, ws, par, offs); break;
        case 9:  processLowerLimiter(n, smps, ws, par, offs); break;
        case 10: processInverseLimiter(n, smps, ws, par, offs); break;
        case 11: processClip(n, smps, ws, par, offs); break;
        case 12: processAsym2(n, smps, ws, offs); break;
        case 13: processPow2(n, smps, ws, offs); break;
        case 14: processSigmoid(n, smps, ws, offs); break;
        case 15: processTanhLimiter(n, smps, ws, par, offs); break;
        case 16: processCubic(n, smps, ws, offs); break;
        case 17: processSquare(n, smps, ws, offs); break;
        default: break; // Optionally handle unknown types
    }
}

} // namespace zyn
