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

// Constants for magic numbers
constexpr float MAX_DRIVE = 127.0f;
constexpr float OFFSET_CENTER = 64.0f;
constexpr float OFFSET_SCALE = 64.0f;
constexpr float FUNC_PAR_SCALE = 127.0f;

// PolyBLAMP residual function
float polyblampres(float smp, float ws, float dMax)
{
    // Formula from: Esqueda, Välimäki, Bilbao (2015): ALIASING REDUCTION IN SOFT-CLIPPING ALGORITHMS
    // http://dafx16.vutbr.cz/dafxpapers/18-DAFx-16_paper_33-PN.pdf pg 123, table 1
    // Four-point polyBLAMP residual:
    // [−2T, T] d^5/120
    // [−T, 0] −d^5/40 + d^4/24 + d^3/12 + d^2/12 + d/24 + 1/120
    // [0, T] d^5/40 − d^4/12 + d^2/3 − d/2 + 7/30
    // [T, 2T] −d^5/120 + d^4/24 − d^3/12 + d^2/12 − d/24 + 1/120
    if (dMax == 0.0f) return 0.0f;

    float dist = std::fabs(smp) - ws;
    float res = 0.0f, d1, d2, d3, d4, d5;

    if (std::fabs(dist) < dMax) {
        if (dist < -dMax / 2.0f) {
            d1 = (dist + dMax) / dMax * 2.0f;
            res = d1 * d1 * d1 * d1 * d1 / 120.0f;
        } else if (dist < 0.0f) {
            d1 = (dist + dMax / 2.0f) / dMax * 2.0f;
            d2 = d1 * d1;
            d3 = d2 * d1;
            d4 = d3 * d1;
            d5 = d4 * d1;
            res = (-d5 / 40.0f) + (d4 / 24.0f) + (d3 / 12.0f) + (d2 / 12.0f) + (d1 / 24.0f) + (1.0f / 120.0f);
        } else if (dist < dMax / 2.0f) {
            d1 = dist / dMax * 2.0f;
            d2 = d1 * d1;
            d4 = d2 * d2;
            d5 = d4 * d1;
            res = (d5 / 40.0f) - (d4 / 12.0f) + (d2 / 3.0f) - (d1 / 2.0f) + (7.0f / 30.0f);
        } else {
            d1 = (dist - dMax / 2.0f) / dMax * 2.0f;
            d2 = d1 * d1;
            d3 = d2 * d1;
            d4 = d3 * d1;
            d5 = d4 * d1;
            res = (-d5 / 120.0f) + (d4 / 24.0f) - (d3 / 12.0f) + (d2 / 12.0f) - (d1 / 24.0f) + (1.0f / 120.0f);
        }
    }
    return res * dMax / 2.0f;
}

// Type-specific distortion functions
namespace {

void processArctangent(int n, float* smps, float ws, float offs) {
    ws = std::pow(10.0f, ws * ws * 3.0f) - 1.0f + 0.001f;
    for (int i = 0; i < n; ++i) {
        smps[i] += offs;
        smps[i] = std::atan(smps[i] * ws) / std::atan(ws);
        smps[i] -= offs;
    }
}

void processAsymmetric(int n, float* smps, float ws, float offs) {
    ws = ws * ws * 32.0f + 0.0001f;
    float tmpv = (ws < 1.0f) ? std::sin(ws) + 0.1f : 1.1f;
    for (int i = 0; i < n; ++i)
        smps[i] = (std::sin((smps[i] + offs) * (0.1f + ws - ws * (smps[i] + offs))) - std::sin(offs * (0.1f + ws - ws * offs))) / tmpv;
}

void processPow(int n, float* smps, float ws, float offs) {
    ws = ws * ws * ws * 20.0f + 0.0001f;
    for (int i = 0; i < n; ++i) {
        float x = (smps[i] + offs) * ws;
        float xo = offs * ws;
        float v = (std::fabs(x) < 1.0f) ? (x - x * x * x) * 3.0f : 0.0f;
        float vo = (std::fabs(xo) < 1.0f) ? (xo - xo * xo * xo) * 3.0f : 0.0f;
        if (ws < 1.0f) {
            v /= ws;
            vo /= ws;
        }
        smps[i] = v - vo;
    }
}

void processSine(int n, float* smps, float ws) {
    ws = ws * ws * ws * 32.0f + 0.0001f;
    float tmpv = (ws < 1.57f) ? std::sin(ws) : 1.0f;
    for (int i = 0; i < n; ++i)
        smps[i] = std::sin(smps[i] * ws) / tmpv;
}

void processQuantize(int n, float* smps, float ws) {
    ws = ws * ws + 0.000001f;
    for (int i = 0; i < n; ++i)
        smps[i] = std::floor(smps[i] / ws + 0.5f) * ws;
}

void processZigzag(int n, float* smps, float ws) {
    ws = ws * ws * ws * 32.0f + 0.0001f;
    float tmpv = (ws < 1.0f) ? std::sin(ws) : 1.0f;
    for (int i = 0; i < n; ++i)
        smps[i] = std::asin(std::sin(smps[i] * ws)) / tmpv;
}

void processLimiter(int n, float* smps, float ws, float par, float offs) {
    ws = std::pow(2.0f, -ws * ws * 8.0f);
    par = par / 4.0f;
    if (par > ws - 0.01f) par = ws - 0.01f;
    for (int i = 0; i < n; ++i) {
        smps[i] += offs;
        float res = polyblampres(smps[i], ws, par);
        if (smps[i] >= 0)
            smps[i] = (smps[i] > ws ? ws - res : smps[i] - res);
        else
            smps[i] = (smps[i] < -ws ? -ws + res : smps[i] + res);
        res = polyblampres(offs, ws, par);
        if (offs >= 0)
            smps[i] -= (offs >= ws ? ws - res : offs - res);
        else
            smps[i] -= (offs <= -ws ? -ws + res : offs + res);
        smps[i] /= ws;
    }
}

void processUpperLimiter(int n, float* smps, float ws, float par, float offs) {
    ws = std::pow(2.0f, -ws * ws * 8.0f);
    if (par > ws - 0.01f) par = ws - 0.01f;
    for (int i = 0; i < n; ++i) {
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


void processLowerLimiter(int n, float* smps, float ws, float par, float offs) {
    ws = std::pow(2.0f, -ws * ws * 8.0f);
    if (par > ws - 0.01f) par = ws - 0.01f;
    for (int i = 0; i < n; ++i) {
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


void processInverseLimiter(int n, float* smps, float ws, float par, float offs) {
    ws = (std::pow(2.0f, ws * 6.0f) - 1.0f) / std::pow(2.0f, 6.0f);
    if (par > ws - 0.01f) par = ws - 0.01f;
    for (int i = 0; i < n; ++i) {
        smps[i] += offs;
        float res = polyblampres(smps[i], ws, par);
        if (smps[i] >= 0)
            smps[i] = (smps[i] > ws ? smps[i] - ws + res : res);
        else
            smps[i] = (smps[i] < -ws ? smps[i] + ws - res : -res);
        smps[i] -= offs;
    }
}

void processClip(int n, float* smps, float ws, float par, float offs) {
    ws = std::pow(5.0f, ws * ws * 1.0f) - 1.0f;
    if (par < 0.0001f) par = 0.0001f; // Verhindere zu kleine Werte
    for (int i = 0; i < n; ++i) {
        smps[i] += offs;
        float x = smps[i] * (ws + 0.5f) * 0.9999f;
        float clipped = x - std::floor(0.5f + x);
        // PolyBLAMP an den Rändern anwenden
        float frac = x - std::floor(x);
        if (frac < par)
            clipped -= polyblampres(frac, 0.0f, par);
        else if (1.0f - frac < par)
            clipped += polyblampres(1.0f - frac, 0.0f, par);
        smps[i] = clipped;
        // Offset rückgängig machen
        float x_offs = offs * (ws + 0.5f) * 0.9999f;
        float clipped_offs = x_offs - std::floor(0.5f + x_offs);
        smps[i] -= clipped_offs;
    }
}

void processAsym2(int n, float* smps, float ws, float offs) {
    ws = ws * ws * ws * 30.0f + 0.001f;
    float tmpv = (ws < 0.3f) ? ws : 1.0f;
    for (int i = 0; i < n; ++i) {
        float tmp = (smps[i] + offs) * ws;
        float tmpo = offs * ws;
        float v = ((tmp > -2.0f) && (tmp < 1.0f)) ? tmp * (1.0f - tmp) * (tmp + 2.0f) : 0.0f;
        float vo = ((tmpo > -2.0f) && (tmpo < 1.0f)) ? tmpo * (1.0f - tmpo) * (tmpo + 2.0f) : 0.0f;
        smps[i] = (v - vo) / tmpv;
    }
}

void processPow2(int n, float* smps, float ws, float offs) {
    ws = ws * ws * ws * 32.0f + 0.0001f;
    float tmpv = (ws < 1.0f) ? ws * (1.0f + ws) / 2.0f : 1.0f;
    for (int i = 0; i < n; ++i) {
        float x = (smps[i] + offs) * ws;
        float xo = offs * ws;
        float v = ((x > -1.0f) && (x < 1.618034f)) ? x * (1.0f - x) : ((x > 0.0f) ? -1.0f : -2.0f);
        float vo = ((xo > -1.0f) && (xo < 1.618034f)) ? xo * (1.0f - xo) : ((xo > 0.0f) ? -1.0f : -2.0f);
        smps[i] = (v - vo) / tmpv;
    }
}

void processSigmoid(int n, float* smps, float ws, float offs) {
    ws = std::pow(ws, 5.0f) * 80.0f + 0.0001f;
    float tmpv = (ws > 10.0f) ? 0.5f : 0.5f - 1.0f / (std::exp(ws) + 1.0f);
    for (int i = 0; i < n; ++i) {
        smps[i] += offs;
        float tmp = smps[i] * ws;
        if (tmp < -10.0f) tmp = -10.0f;
        else if (tmp > 10.0f) tmp = 10.0f;
        tmp = 0.5f - 1.0f / (std::exp(tmp) + 1.0f);

        float tmpo = offs * ws;
        if (tmpo < -10.0f) tmpo = -10.0f;
        else if (tmpo > 10.0f) tmpo = 10.0f;
        tmpo = 0.5f - 1.0f / (std::exp(tmpo) + 1.0f);

        smps[i] = tmp / tmpv - tmpo / tmpv;
    }
}

void processTanhLimiter(int n, float* smps, float ws, float par, float offs) {
    par = (20.0f) * par * par + (0.1f) * par + 1.0f;
    ws = ws * ws * 35.0f + 1.0f;
    for (int i = 0; i < n; ++i) {
        smps[i] *= ws;
        smps[i] += offs;
        smps[i] = smps[i] / std::pow(1.0f + std::pow(std::fabs(smps[i]), par), 1.0f / par);
        smps[i] -= offs / std::pow(1.0f + std::pow(std::fabs(offs), par), 1.0f / par);
    }
}

void processCubic(int n, float* smps, float ws, float offs) {
    ws = ws * ws * ws * 20.0f + 0.168f;
    for (int i = 0; i < n; ++i) {
        smps[i] *= ws;
        smps[i] += offs;
        if (std::fabs(smps[i]) < 1.0f)
            smps[i] = 1.5f * (smps[i] - (smps[i] * smps[i] * smps[i] / 3.0f));
        else
            smps[i] = (smps[i] > 0 ? 1.0f : -1.0f);
        smps[i] -= 1.5f * (offs - (offs * offs * offs / 3.0f));
    }
}

void processSquare(int n, float* smps, float ws, float offs) {
    ws = ws * ws * ws * 20.0f + 0.168f;
    for (int i = 0; i < n; ++i) {
        smps[i] *= ws;
        smps[i] += offs;
        if (std::fabs(smps[i]) < 1.0f)
            smps[i] = smps[i] * (2.0f - std::fabs(smps[i]));
        else
            smps[i] = (smps[i] > 0 ? 1.0f : -1.0f);
        smps[i] -= offs * (2.0f - std::fabs(offs));
    }
}

} // anonymous namespace

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
