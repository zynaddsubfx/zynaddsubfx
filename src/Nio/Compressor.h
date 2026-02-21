/*
   ZynAddSubFX - a software synthesizer

   Compressor.h - simple audio compressor macros
   Copyright (C) 2016-2021 Hans Petter Selasky

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef _COMPRESSOR_H_
#define	_COMPRESSOR_H_

static inline bool floatIsValid(float x)
{
    const float r = x * 0.0f;
    return (r == 0.0f);
}

static inline void stereoCompressor(const int div, float &peak, float &l, float &r)
{
    /*
     * Don't max the output range to avoid
     * overflowing sample rate conversion and
     * equalizer filters in the DSP's output
     * path. Keep one 10th, 1dB, reserved.
     */
    constexpr float limit = 1.0f - (1.0f / 10.0f);

    /* sanity checks */
    if (!floatIsValid(peak))
        peak = 0.0f;
    if (!floatIsValid(l))
        l = 0.0f;
    if (!floatIsValid(r))
        r = 0.0f;
    /* compute maximum */
    if (l < -peak)
        peak = -l;
    else if (l > peak)
        peak = l;
    if (r < -peak)
        peak = -r;
    else if (r > peak)
        peak = r;
    /* compressor */
    if (peak > limit) {
        l /= peak;
        r /= peak;
        l *= limit;
        r *= limit;
        peak -= peak / div;
    }
}

#endif			/* _COMPRESSOR_H_ */
