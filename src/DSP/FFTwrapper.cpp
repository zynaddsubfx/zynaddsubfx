/*
  ZynAddSubFX - a software synthesizer

  FFTwrapper.c  -  A wrapper for Fast Fourier Transforms
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include <cassert>
#include <cstring>
#include <mutex>
#include "FFTwrapper.h"

namespace zyn {

static std::mutex mutex;


FFTwrapper::FFTwrapper(int fftsize_) : m_fftsize(fftsize_)
{
    time      = new fftwf_real[m_fftsize];
    fft       = new fftwf_complex[m_fftsize + 1];

    {
        std::lock_guard<std::mutex> guard(mutex);
        planfftw = fftwf_plan_dft_r2c_1d(m_fftsize,
                                        time,
                                        fft,
                                        FFTW_ESTIMATE);
        planfftw_inv = fftwf_plan_dft_c2r_1d(m_fftsize,
                                            fft,
                                            time,
                                            FFTW_ESTIMATE);
    }
}

FFTwrapper::~FFTwrapper()
{
    {
        std::lock_guard<std::mutex> guard(mutex);
        fftwf_destroy_plan(planfftw);
        fftwf_destroy_plan(planfftw_inv);
    }

    delete [] time;
    delete [] fft;
}

void FFTwrapper::smps2freqs(const FFTsampleBuffer smps, FFTfreqBuffer freqs, FFTsampleBuffer scratch) const
{
    //Load data
    memcpy((void *)scratch.data, (const void *)smps.data, m_fftsize * sizeof(float));

    smps2freqs_noconst_input(scratch, freqs);
}

void FFTwrapper::smps2freqs_noconst_input(FFTsampleBuffer smps, FFTfreqBuffer freqs) const
{
    static_assert (sizeof(float) == sizeof(fftwf_real), "sizeof(float) mismatch");
    assert(m_fftsize == freqs.fftsize);
    assert(m_fftsize == smps.fftsize);

    //DFT
    fftwf_execute_dft_r2c(planfftw,
                          static_cast<fftwf_real*>(smps.data),
                          reinterpret_cast<fftwf_complex*>(freqs.data));
}


void FFTwrapper::freqs2smps(const FFTfreqBuffer freqs, FFTsampleBuffer smps, FFTfreqBuffer scratch) const
{
    //Load data
    memcpy((void *)scratch.data, (const void *)freqs.data, m_fftsize * sizeof(float));

    freqs2smps_noconst_input(scratch, smps);
}


void FFTwrapper::freqs2smps_noconst_input(FFTfreqBuffer freqs, FFTsampleBuffer smps) const
{
    static_assert (sizeof(fft_t) == sizeof(fftwf_complex), "sizeof(complex) mismatch");
    assert(m_fftsize == freqs.fftsize);
    assert(m_fftsize == smps.fftsize);
    fftwf_complex* freqs_complex = reinterpret_cast<fftwf_complex*>(freqs.data);

    //Clear unused freq channel
    freqs_complex[m_fftsize / 2][0] = 0.0f;
    freqs_complex[m_fftsize / 2][1] = 0.0f;

    //IDFT
    fftwf_execute_dft_c2r(planfftw_inv, freqs_complex, smps.data);
}

void FFT_cleanup()
{
    fftwf_cleanup();
}

}
