/*
  ZynAddSubFX - a software synthesizer

  basefunctions.h - the OscilGen base functions
  Copyright (C) 2016-2018 Johannes Lorenz

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <stdexcept>
#include <cassert>
#include <cstdlib>
#include <cmath>
#include <ccomplex>

#include "globals.h"
#include "basefunctions.h"

namespace zyn {

//Define basic functions
#define FUNC(b) float basefunc_ ## b(float x, float a)

FUNC(pulse)
{
    return (fmod(x, 1.0f) < a) ? -1.0f : 1.0f;
}

FUNC(saw)
{
    if(a < 0.00001f)
        a = 0.00001f;
    else
    if(a > 0.99999f)
        a = 0.99999f;
    x = fmod(x, 1);
    if(x < a)
        return x / a * 2.0f - 1.0f;
    else
        return (1.0f - x) / (1.0f - a) * 2.0f - 1.0f;
}

FUNC(triangle)
{
    x = fmod(x + 0.25f, 1);
    a = 1 - a;
    if(a < 0.00001f)
        a = 0.00001f;
    if(x < 0.5f)
        x = x * 4 - 1.0f;
    else
        x = (1.0f - x) * 4 - 1.0f;
    x /= -a;
    if(x < -1.0f)
        x = -1.0f;
    if(x > 1.0f)
        x = 1.0f;
    return x;
}

FUNC(power)
{
    x = fmod(x, 1);
    if(a < 0.00001f)
        a = 0.00001f;
    else
    if(a > 0.99999f)
        a = 0.99999f;
    return powf(x, expf((a - 0.5f) * 10.0f)) * 2.0f - 1.0f;
}

FUNC(gauss)
{
    x = fmod(x, 1) * 2.0f - 1.0f;
    if(a < 0.00001f)
        a = 0.00001f;
    return expf(-x * x * (expf(a * 8) + 5.0f)) * 2.0f - 1.0f;
}

FUNC(diode)
{
    if(a < 0.00001f)
        a = 0.00001f;
    else
    if(a > 0.99999f)
        a = 0.99999f;
    a = a * 2.0f - 1.0f;
    x = cosf((x + 0.5f) * 2.0f * PI) - a;
    if(x < 0.0f)
        x = 0.0f;
    return x / (1.0f - a) * 2 - 1.0f;
}

FUNC(abssine)
{
    x = fmod(x, 1);
    if(a < 0.00001f)
        a = 0.00001f;
    else
    if(a > 0.99999f)
        a = 0.99999f;
    return sinf(powf(x, expf((a - 0.5f) * 5.0f)) * PI) * 2.0f - 1.0f;
}

FUNC(pulsesine)
{
    if(a < 0.00001f)
        a = 0.00001f;
    x = (fmod(x, 1) - 0.5f) * expf((a - 0.5f) * logf(128));
    if(x < -0.5f)
        x = -0.5f;
    else
    if(x > 0.5f)
        x = 0.5f;
    x = sinf(x * PI * 2.0f);
    return x;
}

FUNC(stretchsine)
{
    x = fmod(x + 0.5f, 1) * 2.0f - 1.0f;
    a = (a - 0.5f) * 4;
    if(a > 0.0f)
        a *= 2;
    a = powf(3.0f, a);
    float b = powf(fabs(x), a);
    if(x < 0)
        b = -b;
    return -sinf(b * PI);
}

FUNC(chirp)
{
    x = fmod(x, 1.0f) * 2.0f * PI;
    a = (a - 0.5f) * 4;
    if(a < 0.0f)
        a *= 2.0f;
    a = powf(3.0f, a);
    return sinf(x / 2.0f) * sinf(a * x * x);
}

FUNC(absstretchsine)
{
    x = fmod(x + 0.5f, 1) * 2.0f - 1.0f;
    a = (a - 0.5f) * 9;
    a = powf(3.0f, a);
    float b = powf(fabs(x), a);
    if(x < 0)
        b = -b;
    return -powf(sinf(b * PI), 2);
}

FUNC(chebyshev)
{
    a = a * a * a * 30.0f + 1.0f;
    return cosf(acosf(x * 2.0f - 1.0f) * a);
}

FUNC(sqr)
{
    a = a * a * a * a * 160.0f + 0.001f;
    return -atanf(sinf(x * 2.0f * PI) * a);
}

FUNC(spike)
{
    float b = a * 0.66666; // the width of the range: if a == 0.5, b == 0.33333

    if(x < 0.5) {
        if(x < (0.5 - (b / 2.0)))
            return 0.0;
        else {
            x = (x + (b / 2) - 0.5) * (2.0 / b); // shift to zero, and expand to range from 0 to 1
            return x * (2.0 / b); // this is the slope: 1 / (b / 2)
        }
    }
    else {
        if(x > (0.5 + (b / 2.0)))
            return 0.0;
        else {
            x = (x - 0.5) * (2.0 / b);
            return (1 - x) * (2.0 / b);
        }
    }
}

FUNC(circle)
{
    // a is parameter: 0 -> 0.5 -> 1 // O.5 = circle
    float b, y;

    b = 2 - (a * 2); // b goes from 2 to 0
    x = x * 4;

    if(x < 2) {
        x = x - 1; // x goes from -1 to 1
        if((x < -b) || (x > b))
            y = 0;
        else
            y = sqrt(1 - (pow(x, 2) / pow(b, 2)));  // normally * a^2, but a stays 1
    }
    else {
        x = x - 3; // x goes from -1 to 1 as well
        if((x < -b) || (x > b))
            y = 0;
        else
            y = -sqrt(1 - (pow(x, 2) / pow(b, 2)));
    }
    return y;
}

base_func getBaseFunction(unsigned char func)
{
    if(!func)
        return NULL;

    if(func == 127) //should be the custom wave
        return NULL;

    func--;
    assert(func < 15);
    base_func functions[] = {
        basefunc_triangle,
        basefunc_pulse,
        basefunc_saw,
        basefunc_power,
        basefunc_gauss,
        basefunc_diode,
        basefunc_abssine,
        basefunc_pulsesine,
        basefunc_stretchsine,
        basefunc_chirp,
        basefunc_absstretchsine,
        basefunc_chebyshev,
        basefunc_sqr,
        basefunc_spike,
        basefunc_circle,
    };
    return functions[func];
}

//And filters

#define FILTER(x) float osc_ ## x(unsigned int i, float par, float par2)
FILTER(lp)
{
    float gain = powf(1.0f - par * par * par * 0.99f, i);
    float tmp  = par2 * par2 * par2 * par2 * 0.5f + 0.0001f;
    if(gain < tmp)
        gain = powf(gain, 10.0f) / powf(tmp, 9.0f);
    return gain;
}

FILTER(hp1)
{
    float gain = 1.0f - powf(1.0f - par * par, i + 1);
    return powf(gain, par2 * 2.0f + 0.1f);
}

FILTER(hp1b)
{
    if(par < 0.2f)
        par = par * 0.25f + 0.15f;
    float gain = 1.0f - powf(1.0f - par * par * 0.999f + 0.001f,
                             i * 0.05f * i + 1.0f);
    float tmp = powf(5.0f, par2 * 2.0f);
    return powf(gain, tmp);
}

FILTER(bp1)
{
    float gain = i + 1 - powf(2, (1.0f - par) * 7.5f);
    gain = 1.0f / (1.0f + gain * gain / (i + 1.0f));
    float tmp = powf(5.0f, par2 * 2.0f);
    gain = powf(gain, tmp);
    if(gain < 1e-5)
        gain = 1e-5;
    return gain;
}

FILTER(bs1)
{
    float gain = i + 1 - powf(2, (1.0f - par) * 7.5f);
    gain = powf(atanf(gain / (i / 10.0f + 1)) / 1.57f, 6);
    return powf(gain, par2 * par2 * 3.9f + 0.1f);
}

FILTER(lp2)
{
    return (i + 1 >
            powf(2, (1.0f - par) * 10) ? 0.0f : 1.0f) * par2 + (1.0f - par2);
}

FILTER(hp2)
{
    if(par == 1)
        return 1.0f;
    return (i + 1 >
            powf(2, (1.0f - par) * 7) ? 1.0f : 0.0f) * par2 + (1.0f - par2);
}

FILTER(bp2)
{
    return (fabs(powf(2,
                      (1.0f
                       - par)
                      * 7)
                 - i) > i / 2 + 1 ? 0.0f : 1.0f) * par2 + (1.0f - par2);
}

FILTER(bs2)
{
    return (fabs(powf(2,
                      (1.0f
                       - par)
                      * 7)
                 - i) < i / 2 + 1 ? 0.0f : 1.0f) * par2 + (1.0f - par2);
}

bool floatEq(float a, float b)
{
    const float fudge = .01;
    return a + fudge > b && a - fudge < b;
}

FILTER(cos)
{
    float tmp = powf(5.0f, par2 * 2.0f - 1.0f);
    tmp = powf(i / 32.0f, tmp) * 32.0f;
    if(floatEq(par2 * 127.0f, 64.0f))
        tmp = i;
    float gain = cosf(par * par * PI / 2.0f * tmp);
    gain *= gain;
    return gain;
}

FILTER(sin)
{
    float tmp = powf(5.0f, par2 * 2.0f - 1.0f);
    tmp = powf(i / 32.0f, tmp) * 32.0f;
    if(floatEq(par2 * 127.0f, 64.0f))
        tmp = i;
    float gain = sinf(par * par * PI / 2.0f * tmp);
    gain *= gain;
    return gain;
}

FILTER(low_shelf)
{
    float p2 = 1.0f - par + 0.2f;
    float x  = i / (64.0f * p2 * p2);
    if(x < 0.0f)
        x = 0.0f;
    else
    if(x > 1.0f)
        x = 1.0f;
    float tmp = powf(1.0f - par2, 2.0f);
    return cosf(x * PI) * (1.0f - tmp) + 1.01f + tmp;
}

FILTER(s)
{
    unsigned int tmp = (int) (powf(2.0f, (1.0f - par) * 7.2f));
    float gain = 1.0f;
    if(i == tmp)
        gain = powf(2.0f, par2 * par2 * 8.0f);
    return gain;
}
#undef FILTER

filter_func getFilter(unsigned char func)
{
    if(!func)
        return NULL;

    func--;
    assert(func < 13);
    filter_func functions[] = {
        osc_lp,
        osc_hp1,
        osc_hp1b,
        osc_bp1,
        osc_bs1,
        osc_lp2,
        osc_hp2,
        osc_bp2,
        osc_bs2,
        osc_cos,
        osc_sin,
        osc_low_shelf,
        osc_s
    };
    return functions[func];
}

// other functions

template<class T>
float rmsValue(T&& smps, size_t N)
{
    float sum = 0.0f;
    for(size_t i = 0; i < N; ++i)
        sum += std::norm(smps[i]);
    if(sum < 1e-6f)
        return 1.0f;  // data is all ~zero, do not amplify noise
    return sqrt(sum / N);
}

float rmsNormalOfBaseFunction(unsigned char func,
    int oscilsize, unsigned char basefuncpar)
{
    struct
    {
        base_func bf;
        int oscilsize;
        float par;
        float operator[](std::size_t i) const {
            if(!bf)
                throw std::logic_error("base_func is NULL");
            float t = i * 1.0f / oscilsize;
            t -= floor(t);
            return bf(t, par);
        }
    } baseFuncCalculator { getBaseFunction(func),
                           oscilsize,
                           (basefuncpar + 0.5f) / 128.0f };

    return rmsValue(baseFuncCalculator, oscilsize);
}

}

