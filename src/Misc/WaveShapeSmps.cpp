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

namespace zyn {

float polyblampres(float smp, float ws, float dMax)
{
    // Formula from: Esqueda, Välimäki, Bilbao (2015): ALIASING REDUCTION IN SOFT-CLIPPING ALGORITHMS
    // http://dafx16.vutbr.cz/dafxpapers/18-DAFx-16_paper_33-PN.pdf pg 123, table 1
    // Four-point polyBLAMP residual:
    // [−2T, T] d^5/120
    // [−T, 0] −d^5/40 + d^4/24 + d^3/12 + d^2/12 + d/24 + 1/120
    // [0, T] d^5/40 − d^4/12 + d^2/3 − d/2 + 7/30
    // [T, 2T] −d^5/120 + d^4/24 − d^3/12 + d^2/12 − d/24 + 1/120
    if (dMax == 0) return 0.0;
    float dist = fabsf(smp) - ws;
    float res, d1, d2, d3, d4, d5;
    if(fabsf(dist) < dMax) {
        if(dist < -dMax/2.0f) {
            d1 = (dist + dMax)/dMax*2.0f;   // [-dMax ... -dMax/2] -> [0 ... 1]
            res = d1 * d1 * d1 * d1 * d1 / 120.0f;
        }
        else if(dist < 0.0f) {
            d1 = (dist + dMax/2.0f)/dMax*2.0f; // [-dMax/2 ... 0] -> [0 ... 1]
            d2 = d1*d1;
            d3 = d2*d1;
            d4 = d3*d1;
            d5 = d4*d1;
            res = (-d5/40.0f) + (d4/24.0f) + (d3/12.0f) + (d2/12.0f) + (d1/24.0f) + (1.0f/120.0f);
        }
        else if(dist < dMax/2.0f) {
            d1 = dist/dMax*2.0f;          //[0 ... dMax/2] -> [0 ... 1]
            d2 = d1*d1;
            d4 = d2*d2;
            d5 = d4*d1;
            res = (d5/40.0f) - (d4/12.0f) + (d2/3.0f) - (d1/2.0f) + (7.0f/30.0f);
        }
        else {
            d1 = (dist - dMax/2.0f)/dMax*2.0f; //[dMax/2 ... dMax] -> [0 ... 1]
            d2 = d1*d1;
            d3 = d2*d1;
            d4 = d3*d1;
            d5 = d4*d1;
            res = (-d5/120.0f) + (d4/24.0f) - (d3/12.0f) + (d2/12.0f) - (d1/24.0f) + (1.0f/120.0f);
        }
    }
    else
        res = 0.0f;

    return res*dMax/2.0f;
}

void waveShapeSmps(int n,
                   float *smps,
                   unsigned char type,
                   unsigned char drive,
                   unsigned char offset,
                   unsigned char funcpar)
{
    int   i;
    float ws = drive / 127.0f;
    float par = funcpar / 127.0f;
    float offs = (offset - 64.0f) / 64.0f;
    float tmpv;

    switch(type) {
        case 1:
            ws = powf(10, ws * ws * 3.0f) - 1.0f + 0.001f; //Arctangent
            for(i = 0; i < n; ++i) {
                smps[i] += offs;
                smps[i] = atanf(smps[i] * ws) / atanf(ws);
                smps[i] -= offs;
            }
            break;
        case 2:
            ws = ws * ws * 32.0f + 0.0001f; //Asymmetric
            if(ws < 1.0f)
                tmpv = sinf(ws) + 0.1f;
            else
                tmpv = 1.1f;
            for(i = 0; i < n; ++i)
                smps[i] = sinf(smps[i] * (0.1f + ws - ws * smps[i])) / tmpv;
            ;
            break;
        case 3:
            ws = ws * ws * ws * 20.0f + 0.0001f; //Pow
            for(i = 0; i < n; ++i) {
                smps[i] *= ws;
                if(fabsf(smps[i]) < 1.0f) {
                    smps[i] = (smps[i] - smps[i] * smps[i] * smps[i]) * 3.0f;
                    if(ws < 1.0f)
                        smps[i] /= ws;
                }
                else
                    smps[i] = 0.0f;
            }
            break;
        case 4:
            ws = ws * ws * ws * 32.0f + 0.0001f; //Sine
            if(ws < 1.57f)
                tmpv = sinf(ws);
            else
                tmpv = 1.0f;
            for(i = 0; i < n; ++i)
                smps[i] = sinf(smps[i] * ws) / tmpv;
            break;
        case 5:
            ws = ws * ws + 0.000001f; //Quantisize
            for(i = 0; i < n; ++i)
                smps[i] = floor(smps[i] / ws + 0.5f) * ws;
            break;
        case 6:
            ws = ws * ws * ws * 32 + 0.0001f; //Zigzag
            if(ws < 1.0f)
                tmpv = sinf(ws);
            else
                tmpv = 1.0f;
            for(i = 0; i < n; ++i)
                smps[i] = asinf(sinf(smps[i] * ws)) / tmpv;
            break;
        case 7:
            ws = powf(2.0f, -ws * ws * 8.0f); //Limiter
            par = par/4;
            if (par > ws - 0.01) par = ws - 0.01;
            for(i = 0; i < n; ++i) {
                // add the offset: x = smps[i] + offs
                smps[i] += offs;
                float res = polyblampres(smps[i], ws, par);
                // now apply the polyblamped limiter: y = f(x)
                if (smps[i]>=0)
                    smps[i] = ( smps[i] > ws ? ws-res : smps[i]-res );
                else
                    smps[i] = ( smps[i] < -ws ? -ws+res : smps[i]+res );
                // and subtract the polyblamp-limited offset again: smps[i] = y - f(offs)
                res = polyblampres(offs, ws, par);
                if (offs>=0)
                    smps[i] -= ( offs >= ws ? ws-res : offs-res );
                else
                    smps[i] -= ( offs <= -ws ? -ws+res : offs+res );
                // divide through the drive factor: prevents limited signals to get low
                smps[i] /= ws;

            }
            break;
        case 8:
            ws = powf(2.0f, -ws * ws * 8.0f); //Upper Limiter
            for(i = 0; i < n; ++i) {
                float tmp = smps[i];
                if(tmp > ws)
                    smps[i] = ws;
                smps[i] *= 2.0f;
            }
            break;
        case 9:
            ws = powf(2.0f, -ws * ws * 8.0f); //Lower Limiter
            for(i = 0; i < n; ++i) {
                float tmp = smps[i];
                if(tmp < -ws)
                    smps[i] = -ws;
                smps[i] *= 2.0f;
            }
            break;
        case 10:
            ws = (powf(2.0f, ws * 6.0f) - 1.0f) / powf(2.0f, 6.0f); //Inverse Limiter
            if (par > ws - 0.01) par = ws - 0.01;
            for(i = 0; i < n; ++i) {
                smps[i] += offs;
                float res = polyblampres(smps[i], ws, par);
                if (smps[i]>=0)
                    smps[i] = ( smps[i] > ws ? smps[i]-ws+res : res );
                else
                    smps[i] = ( smps[i] < -ws ? smps[i]+ws-res : -res );
                smps[i] -= offs;
            }
            break;
        case 11:
            ws = powf(5, ws * ws * 1.0f) - 1.0f; //Clip
            for(i = 0; i < n; ++i)
                smps[i] = smps[i]
                          * (ws + 0.5f) * 0.9999f - floor(
                    0.5f + smps[i] * (ws + 0.5f) * 0.9999f);
            break;
        case 12:
            ws = ws * ws * ws * 30 + 0.001f; //Asym2
            if(ws < 0.3f)
                tmpv = ws;
            else
                tmpv = 1.0f;
            for(i = 0; i < n; ++i) {
                float tmp = smps[i] * ws;
                if((tmp > -2.0f) && (tmp < 1.0f))
                    smps[i] = tmp * (1.0f - tmp) * (tmp + 2.0f) / tmpv;
                else
                    smps[i] = 0.0f;
            }
            break;
        case 13:
            ws = ws * ws * ws * 32.0f + 0.0001f; //Pow2
            if(ws < 1.0f)
                tmpv = ws * (1 + ws) / 2.0f;
            else
                tmpv = 1.0f;
            for(i = 0; i < n; ++i) {
                float tmp = smps[i] * ws;
                if((tmp > -1.0f) && (tmp < 1.618034f))
                    smps[i] = tmp * (1.0f - tmp) / tmpv;
                else
                if(tmp > 0.0f)
                    smps[i] = -1.0f;
                else
                    smps[i] = -2.0f;
            }
            break;
        case 14:
            ws = powf(ws, 5.0f) * 80.0f + 0.0001f; //sigmoid
            if(ws > 10.0f)
                tmpv = 0.5f;
            else
                tmpv = 0.5f - 1.0f / (expf(ws) + 1.0f);
            for(i = 0; i < n; ++i) {
                smps[i] += offs; //add offset
                // calculate sigmoid function
                float tmp = smps[i] * ws;
                if(tmp < -10.0f)
                    tmp = -10.0f;
                else
                if(tmp > 10.0f)
                    tmp = 10.0f;
                tmp     = 0.5f - 1.0f / (expf(tmp) + 1.0f);
                // calculate the same for offset value
                float tmpo = offs * ws;
                if(tmpo < -10.0f)
                    tmpo = -10.0f;
                else
                if(tmpo > 10.0f)
                    tmpo = 10.0f;
                tmpo     = 0.5f - 1.0f / (expf(tmpo) + 1.0f);

                smps[i] = tmp / tmpv;
                smps[i] -= tmpo / tmpv; // subtract offset
            }
            break;
        case 15: // tanh soft limiter
            // f(x) = x / ((1+|x|^n)^(1/n)) // tanh approximation for n=2.5
            // Formula from: Yeh, Abel, Smith (2007): SIMPLIFIED, PHYSICALLY-INFORMED MODELS OF DISTORTION AND OVERDRIVE GUITAR EFFECTS PEDALS
            par = (20.0f) * par * par + (0.1f) * par + 1.0f;  //Pfunpar=32 -> n=2.5
            ws = ws * ws * 35.0f + 1.0f;
            for(i = 0; i < n; ++i) {
                smps[i] *= ws;// multiply signal to drive it in the saturation of the function
                smps[i] += offs; // add dc offset
                smps[i] = smps[i] / powf(1+powf(fabsf(smps[i]), par), 1/par);
                smps[i] -= offs / powf(1+powf(fabsf(offs), par), 1/par);
            }
            break;
        case 16: //cubic distortion
            // f(x) = 1.5 * (x-(x^3/3))
            // Formula from: https://ccrma.stanford.edu/~jos/pasp/Soft_Clipping.html
            // modified with factor 1.5 to go through [1,1] and [-1,-1]
            ws = ws * ws * ws * 20.0f + 0.168f; // plain cubic at drive=44
            for(i = 0; i < n; ++i) {
                smps[i] *= ws; // multiply signal to drive it in the saturation of the function
                smps[i] += offs; // add dc offset
                if(fabsf(smps[i]) < 1.0f)
                    smps[i] = 1.5 * (smps[i] - (smps[i]*smps[i]*smps[i] / 3.0) );
                else
                    smps[i] = (smps[i] > 0 ? 1.0f : -1.0f);
                //subtract offset with distorsion function applied
                smps[i] -= 1.5 * (offs - (offs*offs*offs / 3.0));
            }
            break;
        case 17: //square distortion
        // f(x) = x*(2-abs(x))
        // Formula of cubic changed to square but still going through [1,1] and [-1,-1]
            ws = ws * ws * ws * 20.0f + 0.168f; // plain square at drive=44
            for(i = 0; i < n; ++i) {
                smps[i] *= ws; // multiply signal to drive it in the saturation of the function
                smps[i] += offs; // add dc offset
                if(fabsf(smps[i]) < 1.0f)
                    smps[i] = smps[i]*(2-fabsf(smps[i]));
                else
                    smps[i] = (smps[i] > 0 ? 1.0f : -1.0f);
                //subtract offset with distorsion function applied
                smps[i] -= offs*(2-fabsf(offs));
            }
            break;
    }
}

}
