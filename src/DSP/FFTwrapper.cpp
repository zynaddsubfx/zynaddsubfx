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
#include <pthread.h>
#include "FFTwrapper.h"

namespace zyn {

static pthread_mutex_t *mutex = NULL;

FFTwrapper::FFTwrapper(int fftsize_)
{
    //first one will spawn the mutex (yeah this may be a race itself)
    if(!mutex) {
        mutex = new pthread_mutex_t;
        pthread_mutex_init(mutex, NULL);
    }


    m_fftsize = fftsize_;
    time      = new fftwf_real[m_fftsize];
    fft       = new fftwf_complex[m_fftsize + 1];
    pthread_mutex_lock(mutex);
    planfftw = fftwf_plan_dft_r2c_1d(m_fftsize,
                                    time,
                                    fft,
                                    FFTW_ESTIMATE);
    planfftw_inv = fftwf_plan_dft_c2r_1d(m_fftsize,
                                        fft,
                                        time,
                                        FFTW_ESTIMATE);
    pthread_mutex_unlock(mutex);
}

FFTwrapper::~FFTwrapper()
{
    pthread_mutex_lock(mutex);
    fftwf_destroy_plan(planfftw);
    fftwf_destroy_plan(planfftw_inv);
    pthread_mutex_unlock(mutex);

    delete [] time;
    delete [] fft;
}

void FFTwrapper::smps2freqs(const float *smps, fft_t *freqs)
{
    //Load data
    memcpy((void *)time, (const void *)smps, m_fftsize * sizeof(float));

    //DFT
    fftwf_execute(planfftw);

    //Grab data
    memcpy((void *)freqs, (const void *)fft, m_fftsize * sizeof(float));
}

void FFTwrapper::freqs2smps(const fft_t *freqs, float *smps)
{
    //Load data
    memcpy((void *)fft, (const void *)freqs, m_fftsize * sizeof(float));

    //clear unused freq channel
    fft[m_fftsize / 2][0] = 0.0f;
    fft[m_fftsize / 2][1] = 0.0f;

    //IDFT
    fftwf_execute(planfftw_inv);

    //Grab data
    memcpy((void*)smps, (const void*)time, m_fftsize * sizeof(float));
}

void FFT_cleanup()
{
    fftwf_cleanup();
    pthread_mutex_destroy(mutex);
    delete mutex;
    mutex = NULL;
}

}
