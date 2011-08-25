/*
  ZynAddSubFX - a software synthesizer

  WaveShapeSmps.cpp - Sample Distortion
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include "WaveShapeSmps.h"
#include <cmath>

void waveShapeSmps(int n,
                   float *smps,
                   unsigned char type,
                   unsigned char drive)
{
    int      i;
    float ws = drive / 127.0;
    float tmpv;

    switch(type) {
    case 1:
        ws = pow(10, ws * ws * 3.0) - 1.0 + 0.001; //Arctangent
        for(i = 0; i < n; ++i)
            smps[i] = atan(smps[i] * ws) / atan(ws);
        break;
    case 2:
        ws = ws * ws * 32.0 + 0.0001; //Asymmetric
        if(ws < 1.0)
            tmpv = sin(ws) + 0.1;
        else
            tmpv = 1.1;
        for(i = 0; i < n; ++i)
            smps[i] = sin(smps[i] * (0.1 + ws - ws * smps[i])) / tmpv;
        ;
        break;
    case 3:
        ws = ws * ws * ws * 20.0 + 0.0001; //Pow
        for(i = 0; i < n; ++i) {
            smps[i] *= ws;
            if(fabs(smps[i]) < 1.0) {
                smps[i] = (smps[i] - pow(smps[i], 3.0)) * 3.0;
                if(ws < 1.0)
                    smps[i] /= ws;
            }
            else
                smps[i] = 0.0;
        }
        break;
    case 4:
        ws = ws * ws * ws * 32.0 + 0.0001; //Sine
        if(ws < 1.57)
            tmpv = sin(ws);
        else
            tmpv = 1.0;
        for(i = 0; i < n; ++i)
            smps[i] = sin(smps[i] * ws) / tmpv;
        break;
    case 5:
        ws = ws * ws + 0.000001; //Quantisize
        for(i = 0; i < n; ++i)
            smps[i] = floor(smps[i] / ws + 0.5) * ws;
        break;
    case 6:
        ws = ws * ws * ws * 32 + 0.0001; //Zigzag
        if(ws < 1.0)
            tmpv = sin(ws);
        else
            tmpv = 1.0;
        for(i = 0; i < n; ++i)
            smps[i] = asin(sin(smps[i] * ws)) / tmpv;
        break;
    case 7:
        ws = pow(2.0, -ws * ws * 8.0); //Limiter
        for(i = 0; i < n; ++i) {
            float tmp = smps[i];
            if(fabs(tmp) > ws) {
                if(tmp >= 0.0)
                    smps[i] = 1.0;
                else
                    smps[i] = -1.0;
            }
            else
                smps[i] /= ws;
        }
        break;
    case 8:
        ws = pow(2.0, -ws * ws * 8.0); //Upper Limiter
        for(i = 0; i < n; ++i) {
            float tmp = smps[i];
            if(tmp > ws)
                smps[i] = ws;
            smps[i] *= 2.0;
        }
        break;
    case 9:
        ws = pow(2.0, -ws * ws * 8.0); //Lower Limiter
        for(i = 0; i < n; ++i) {
            float tmp = smps[i];
            if(tmp < -ws)
                smps[i] = -ws;
            smps[i] *= 2.0;
        }
        break;
    case 10:
        ws = (pow(2.0, ws * 6.0) - 1.0) / pow(2.0, 6.0); //Inverse Limiter
        for(i = 0; i < n; ++i) {
            float tmp = smps[i];
            if(fabs(tmp) > ws) {
                if(tmp >= 0.0)
                    smps[i] = tmp - ws;
                else
                    smps[i] = tmp + ws;
            }
            else
                smps[i] = 0;
        }
        break;
    case 11:
        ws = pow(5, ws * ws * 1.0) - 1.0; //Clip
        for(i = 0; i < n; ++i)
            smps[i] = smps[i]
                      * (ws + 0.5) * 0.9999 - floor(
                0.5 + smps[i] * (ws + 0.5) * 0.9999);
        break;
    case 12:
        ws = ws * ws * ws * 30 + 0.001; //Asym2
        if(ws < 0.3)
            tmpv = ws;
        else
            tmpv = 1.0;
        for(i = 0; i < n; ++i) {
            float tmp = smps[i] * ws;
            if((tmp > -2.0) && (tmp < 1.0))
                smps[i] = tmp * (1.0 - tmp) * (tmp + 2.0) / tmpv;
            else
                smps[i] = 0.0;
        }
        break;
    case 13:
        ws = ws * ws * ws * 32.0 + 0.0001; //Pow2
        if(ws < 1.0)
            tmpv = ws * (1 + ws) / 2.0;
        else
            tmpv = 1.0;
        for(i = 0; i < n; ++i) {
            float tmp = smps[i] * ws;
            if((tmp > -1.0) && (tmp < 1.618034))
                smps[i] = tmp * (1.0 - tmp) / tmpv;
            else
            if(tmp > 0.0)
                smps[i] = -1.0;
            else
                smps[i] = -2.0;
        }
        break;
    case 14:
        ws = pow(ws, 5.0) * 80.0 + 0.0001; //sigmoid
        if(ws > 10.0)
            tmpv = 0.5;
        else
            tmpv = 0.5 - 1.0 / (exp(ws) + 1.0);
        for(i = 0; i < n; ++i) {
            float tmp = smps[i] * ws;
            if(tmp < -10.0)
                tmp = -10.0;
            else
            if(tmp > 10.0)
                tmp = 10.0;
            tmp     = 0.5 - 1.0 / (exp(tmp) + 1.0);
            smps[i] = tmp / tmpv;
        }
        break;
    }
}
