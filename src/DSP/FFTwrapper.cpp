/*
  ZynAddSubFX - a software synthesizer

  FFTwrapper.c  -  A wrapper for Fast Fourier Transforms
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

#include <cmath>
#include <cassert>
#include "FFTwrapper.h"

FFTwrapper::FFTwrapper(int fftsize_)
{
    fftsize      = fftsize_;
    tmpfftdata1  = new fftw_real[fftsize];
    tmpfftdata2  = new fftw_real[fftsize];
    planfftw     = fftw_plan_r2r_1d(fftsize,
                                    tmpfftdata1,
                                    tmpfftdata1,
                                    FFTW_R2HC,
                                    FFTW_ESTIMATE);
    planfftw_inv = fftw_plan_r2r_1d(fftsize,
                                    tmpfftdata2,
                                    tmpfftdata2,
                                    FFTW_HC2R,
                                    FFTW_ESTIMATE);
}

FFTwrapper::~FFTwrapper()
{
    fftw_destroy_plan(planfftw);
    fftw_destroy_plan(planfftw_inv);

    delete [] tmpfftdata1;
    delete [] tmpfftdata2;
}

/*
 * do the Fast Fourier Transform
 */
void FFTwrapper::smps2freqs(const float *smps, FFTFREQS freqs)
{
    for(int i = 0; i < fftsize; i++)
        tmpfftdata1[i] = smps[i];
    fftw_execute(planfftw);
    for(int i = 0; i < fftsize / 2; i++) {
        freqs.c[i] = tmpfftdata1[i];
        if(i != 0)
            freqs.s[i] = tmpfftdata1[fftsize - i];
    }
    tmpfftdata2[fftsize / 2] = 0.0;
}

/*
 * do the Inverse Fast Fourier Transform
 */
void FFTwrapper::freqs2smps(const FFTFREQS freqs, float *smps)
{
    tmpfftdata2[fftsize / 2] = 0.0;
    for(int i = 0; i < fftsize / 2; i++) {
        tmpfftdata2[i] = freqs.c[i];
        if(i != 0)
            tmpfftdata2[fftsize - i] = freqs.s[i];
    }
    fftw_execute(planfftw_inv);
    for(int i = 0; i < fftsize; i++)
        smps[i] = tmpfftdata2[i];
}

//only OSCILLGEN SHOULD CALL THIS FOR NOW
void FFTwrapper::smps2freqs(const float *smps, fft_t *freqs)
{
    assert(fftsize==OSCIL_SIZE);
    FFTFREQS tmp;
    newFFTFREQS(&tmp, fftsize);
    smps2freqs(smps, tmp);
    for(int i = 0; i < fftsize / 2; ++i)
        freqs[i] = fft_t(tmp.c[i], tmp.s[i]);
    deleteFFTFREQS(&tmp);
}

void FFTwrapper::freqs2smps(const fft_t *freqs, float *smps)
{
    assert(fftsize==OSCIL_SIZE);
    FFTFREQS tmp;
    newFFTFREQS(&tmp, fftsize);
    for(int i = 0; i < fftsize / 2; ++i) {
        tmp.c[i] = freqs[i].real();
        tmp.s[i] = freqs[i].imag();
    }
    freqs2smps(tmp, smps);
    deleteFFTFREQS(&tmp);
}

void newFFTFREQS(FFTFREQS *f, int size)
{
    f->c = new float[size];
    f->s = new float[size];
    for(int i = 0; i < size; i++) {
        f->c[i] = 0.0;
        f->s[i] = 0.0;
    }
}

void deleteFFTFREQS(FFTFREQS *f)
{
    delete[] f->c;
    delete[] f->s;
    f->c = f->s = NULL;
}

void FFT_cleanup()
{
    fftw_cleanup();
}

