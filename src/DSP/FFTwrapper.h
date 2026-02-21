/*
  ZynAddSubFX - a software synthesizer

  FFTwrapper.h  -  A wrapper for Fast Fourier Transforms
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef FFT_WRAPPER_H
#define FFT_WRAPPER_H
#include <fftw3.h>
#include <complex>
#include "../globals.h"

namespace zyn {

//! Struct to make sure FFT sizes fit. *Not* an RAII class
struct FFTfreqBuffer
{
    friend class FFTwrapper;

    const int fftsize;
    fft_t* data;

    // comfort functions
    fft_t& operator[](std::size_t idx) { return data[idx]; }
    const fft_t& operator[](std::size_t idx) const { return data[idx]; }
    //! Allocation size. For users of this class, `fftsize/2` would be enough,
    //! But fftw needs `fftsize+1` freqs to operate on
    int allocSize() const { return fftsize + 1; }

private:
    FFTfreqBuffer(int fftsize, fft_t* ptr = nullptr) : // called by FFTwrapper
        fftsize(fftsize),
        data(ptr ? ptr : new fft_t[allocSize()])
    {}
};

//! Struct to make sure FFT sizes fit. *Not* an RAII class
struct FFTsampleBuffer
{
    friend class FFTwrapper;

    const int fftsize;
    float* data;

    // comfort functions
    float& operator[](std::size_t idx) { return data[idx]; }
    const float& operator[](std::size_t idx) const { return data[idx]; }
    int allocSize() const { return fftsize; }

private:
    FFTsampleBuffer(int fftsize, float* ptr = nullptr) : // called by FFTwrapper
        fftsize(fftsize),
        data(ptr ? ptr : new float[allocSize()])
    {}
};

/**
    A wrapper for the FFTW library (Fast Fourier Transforms)
    All methods (except CTOR/DTOR) are static/const. This class is thread-safe.
*/
class FFTwrapper
{
    public:
        /**Constructor
         * @param fftsize_ The size of samples to be fed to fftw*/
        FFTwrapper(int fftsize_);
        /**Destructor*/
        ~FFTwrapper();
        /**Convert Samples to Frequencies using Fourier Transform
         * @param smps Pointer to Samples to be converted; has length fftsize_
         * @param freqs Structure FFTFREQS which stores the frequencies*/
        void smps2freqs(const FFTsampleBuffer smps, FFTfreqBuffer freqs, FFTsampleBuffer scratch) const;
        void freqs2smps(const FFTfreqBuffer freqs, FFTsampleBuffer smps, FFTfreqBuffer scratch) const;
        void smps2freqs_noconst_input(FFTsampleBuffer smps, FFTfreqBuffer freqs) const;
        void freqs2smps_noconst_input(FFTfreqBuffer freqs, FFTsampleBuffer smps) const;

        // Whenever you need one of the FFT*Buffers, you take them from here
        // The methods make sure that the FFT*Buffers match the fftsize
        FFTfreqBuffer allocFreqBuf(fft_t* ptr = nullptr) const { return FFTfreqBuffer(m_fftsize, ptr); }
        FFTsampleBuffer allocSampleBuf(float* ptr = nullptr) const { return FFTsampleBuffer(m_fftsize, ptr); }
        // These should only be used exceptionally if you need to alloc those buffers,
        // buf never run any FFT/IFFT on them
        static FFTfreqBuffer riskAllocFreqBufWithSize(int othersize) { return FFTfreqBuffer(othersize); }
        static FFTsampleBuffer riskAllocSampleBufWithSize(int othersize) { return FFTsampleBuffer(othersize); }

        int fftsize() const { return m_fftsize; }

    private:
        const int     m_fftsize;
        fftwf_real    *time; // only used when creating plan
        fftwf_complex *fft;  // only used when creating plan
        fftwf_plan    planfftw, planfftw_inv;
};

/*
 * The "std::polar" template has no clear definition for the range of
 * the input parameters, and some C++ standard library implementations
 * don't accept negative amplitude among others. Define our own
 * FFTpolar template, which works like we expect it to.
 */
template<class _Tp>
std::complex<_Tp>
FFTpolar(const _Tp& __rho, const _Tp& __theta = _Tp(0))
{
        _Tp __x = __rho * std::cos(__theta);
        if (std::isnan(__x))
                __x = 0;
        _Tp __y = __rho * std::sin(__theta);
        if (std::isnan(__y))
                __y = 0;
        return std::complex<_Tp>(__x, __y);
}

void FFT_cleanup();

}

#endif
