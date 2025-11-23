/*
  ZynAddSubFX - a software synthesizer

  AnalogFilter.cpp - Several analog filters (lowpass, highpass...)
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Copyright (C) 2010-2010 Mark McCurry
  Author: Nasca Octavian Paul
          Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cstring> //memcpy
#include <cmath>
#include <cassert>

#include "../Misc/Util.h"
#include "AnalogFilter.h"


const float MAX_FREQ = 20000.0f;
const int windowSize = 8192;

namespace zyn {

AnalogFilter::AnalogFilter(unsigned char Ftype,
                           float Ffreq,
                           float Fq,
                           unsigned char Fstages,
                           unsigned int srate, int bufsize)
    :Filter(srate, bufsize),
      type(Ftype),
      stages(Fstages),
      freq(Ffreq),
      q(Fq),
      newq(Fq),
     gain(1.0),
     recompute(true),
     freqbufsize(bufsize/8)
{
    for(int i = 0; i < 3; ++i)
        coeff.c[i] = coeff.d[i] = oldCoeff.c[i] = oldCoeff.d[i] = 0.0f;
    if(stages >= MAX_FILTER_STAGES)
        stages = MAX_FILTER_STAGES;
    cleanup();
    setfreq_and_q(Ffreq, Fq);
    coeff.d[0] = 0; //this is not used
    outgain    = 1.0f;
    freq_smoothing.sample_rate(samplerate_f/8);
    freq_smoothing.thresh(2.0f); // 2Hz
    beforeFirstTick=true;
}

AnalogFilter::~AnalogFilter()
{}

void AnalogFilter::cleanup()
{
    for(int i = 0; i < MAX_FILTER_STAGES + 1; ++i) {
        history[i].x1 = 0.0f;
        history[i].x2 = 0.0f;
        history[i].y1 = 0.0f;
        history[i].y2 = 0.0f;
        oldHistory[i] = history[i];
    }
}

AnalogFilter::Coeff AnalogFilter::computeCoeff(int type, float cutoff, float q,
        int stages, float gain, float fs, int &order, bool loudnessCompEnabled)
{
    AnalogFilter::Coeff coeff;
    bool  zerocoefs = false; //this is used if the freq is too high

    const float samplerate_f = fs;
    const float halfsamplerate_f = fs/2;

    //do not allow frequencies bigger than samplerate/2
    float freq = cutoff;
    if(freq > (halfsamplerate_f - 500.0f)) {
        freq      = halfsamplerate_f - 500.0f;
        zerocoefs = true;
    }

    if(freq < 0.1f)
        freq = 0.1f;

    //do not allow bogus Q
    if(q < 0.0f)
        q = 0.0f;


    float tmpq, tmpgain;
    if(stages == 0) {
        tmpq    = q;
        tmpgain = gain;
    } else {
        tmpq    = (q > 1.0f) ? powf(q, 1.0f / (stages + 1)) : q;
        tmpgain = powf(gain, 1.0f / (stages + 1));
    }

    //Alias Terms
    float *c = coeff.c;
    float *d = coeff.d;

    //General Constants
    const float omega = 2 * PI * freq / samplerate_f;
    const float sn    = sinf(omega), cs = cosf(omega);
    float       alpha, beta;

    //most of these are implementations of
    //the "Cookbook formulae for audio EQ" by Robert Bristow-Johnson
    //The original location of the Cookbook is:
    //http://www.harmony-central.com/Computer/Programming/Audio-EQ-Cookbook.txt
    float tmp;
    float tgp1;
    float tgm1;
    switch(type) {
        case 0: //LPF 1 pole
            if(!zerocoefs)
                tmp = expf(-2.0f * PI * freq / samplerate_f);
            else
                tmp = 0.0f;
            c[0]  = 1.0f - tmp;
            c[1]  = 0.0f;
            c[2]  = 0.0f;
            d[1]  = tmp;
            d[2]  = 0.0f;
            order = 1;
            break;
        case 1: //HPF 1 pole
            if(!zerocoefs)
                tmp = expf(-2.0f * PI * freq / samplerate_f);
            else
                tmp = 0.0f;
            c[0]  = (1.0f + tmp) / 2.0f;
            c[1]  = -(1.0f + tmp) / 2.0f;
            c[2]  = 0.0f;
            d[1]  = tmp;
            d[2]  = 0.0f;
            order = 1;
            break;
        case 2: //LPF 2 poles
            if(!zerocoefs) {
                alpha = sn / (2.0f * tmpq);
                tmp   = 1 + alpha;
                c[1]  = (1.0f - cs) / tmp;
                c[0]  = c[2] = c[1] / 2.0f;
                d[1]  = -2.0f * cs / tmp * -1.0f;
                d[2]  = (1.0f - alpha) / tmp * -1.0f;
            }
            else {
                c[0] = 1.0f;
                c[1] = c[2] = d[1] = d[2] = 0.0f;
            }
            order = 2;
            break;
        case 3: //HPF 2 poles
            if(!zerocoefs) {
                alpha = sn / (2.0f * tmpq);
                tmp   = 1 + alpha;
                c[0]  = (1.0f + cs) / 2.0f / tmp;
                c[1]  = -(1.0f + cs) / tmp;
                c[2]  = (1.0f + cs) / 2.0f / tmp;
                d[1]  = -2.0f * cs / tmp * -1.0f;
                d[2]  = (1.0f - alpha) / tmp * -1.0f;
            }
            else
                c[0] = c[1] = c[2] = d[1] = d[2] = 0.0f;
            order = 2;
            break;
        case 4: //BPF 2 poles
            if(!zerocoefs) {
                alpha = sn / (2.0f * tmpq);
                tmp   = 1.0f + alpha;
                c[0]  = alpha / tmp *sqrtf(tmpq + 1.0f);
                c[1]  = 0.0f;
                c[2]  = -alpha / tmp *sqrtf(tmpq + 1.0f);
                d[1]  = -2.0f * cs / tmp * -1.0f;
                d[2]  = (1.0f - alpha) / tmp * -1.0f;
            }
            else
                c[0] = c[1] = c[2] = d[1] = d[2] = 0.0f;
            order = 2;
            break;
        case 5: //NOTCH 2 poles
            if(!zerocoefs) {
                alpha = sn / (2.0f * sqrtf(tmpq));
                tmp   = 1.0f + alpha;
                c[0]  = 1.0f / tmp;
                c[1]  = -2.0f * cs / tmp;
                c[2]  = 1.0f / tmp;
                d[1]  = -2.0f * cs / tmp * -1.0f;
                d[2]  = (1.0f - alpha) / tmp * -1.0f;
            }
            else {
                c[0] = 1.0f;
                c[1] = c[2] = d[1] = d[2] = 0.0f;
            }
            order = 2;
            break;
        case 6: //PEAK (2 poles)
            if(!zerocoefs) {
                tmpq *= 3.0f;
                alpha = sn / (2.0f * tmpq);
                tmp   = 1.0f + alpha / tmpgain;
                c[0]  = (1.0f + alpha * tmpgain) / tmp;
                c[1]  = (-2.0f * cs) / tmp;
                c[2]  = (1.0f - alpha * tmpgain) / tmp;
                d[1]  = -2.0f * cs / tmp * -1.0f;
                d[2]  = (1.0f - alpha / tmpgain) / tmp * -1.0f;
            }
            else {
                c[0] = 1.0f;
                c[1] = c[2] = d[1] = d[2] = 0.0f;
            }
            order = 2;
            break;
        case 7: //Low Shelf - 2 poles
            if(!zerocoefs) {
                tmpq  = sqrtf(tmpq);
                beta  = sqrtf(tmpgain) / tmpq;
                tgp1  = tmpgain + 1.0f;
                tgm1  = tmpgain - 1.0f;
                tmp   = tgp1 + tgm1 * cs + beta * sn;

                c[0] = tmpgain * (tgp1 - tgm1 * cs + beta * sn) / tmp;
                c[1] = 2.0f * tmpgain * (tgm1 - tgp1 * cs) / tmp;
                c[2] = tmpgain * (tgp1 - tgm1 * cs - beta * sn) / tmp;
                d[1] = -2.0f * (tgm1 + tgp1 * cs) / tmp * -1.0f;
                d[2] = (tgp1 + tgm1 * cs - beta * sn) / tmp * -1.0f;
            }
            else {
                c[0] = tmpgain;
                c[1] = c[2] = d[1] = d[2] = 0.0f;
            }
            order = 2;
            break;
        case 8: //High Shelf - 2 poles
            if(!zerocoefs) {
                tmpq  = sqrtf(tmpq);
                beta  = sqrtf(tmpgain) / tmpq;
                tgp1  = tmpgain + 1.0f;
                tgm1  = tmpgain - 1.0f;
                tmp   = tgp1 - tgm1 * cs + beta * sn;

                c[0] = tmpgain * (tgp1 + tgm1 * cs + beta * sn) / tmp;
                c[1] = -2.0f * tmpgain * (tgm1 + tgp1 * cs) / tmp;
                c[2] = tmpgain * (tgp1 + tgm1 * cs - beta * sn) / tmp;
                d[1] = 2.0f * (tgm1 - tgp1 * cs) / tmp * -1.0f;
                d[2] = (tgp1 - tgm1 * cs - beta * sn) / tmp * -1.0f;
            }
            else {
                c[0] = 1.0f;
                c[1] = c[2] = d[1] = d[2] = 0.0f;
            }
            order = 2;
            break;
        default: //wrong type
            assert(false && "wrong type for a filter");
            break;
    }

    if (loudnessCompEnabled && type >= 6)
{
    // Bark band center frequencies (24 critical bands of human hearing)
    // These represent the psychoacoustic frequency resolution of the human ear
    const float barkCenterFreq[] = {
        50.0f, 150.0f, 250.0f, 350.0f, 450.0f, 570.0f, 700.0f, 840.0f,
        1000.0f, 1170.0f, 1370.0f, 1600.0f, 1850.0f, 2150.0f, 2500.0f, 2900.0f,
        3400.0f, 4000.0f, 4800.0f, 5800.0f, 7000.0f, 8500.0f, 10500.0f, 13500.0f
    };

    // Bark band bandwidths (approximate ERB - Equivalent Rectangular Bandwidth)
    // These widths correspond to human frequency resolution at each critical band
    const float barkBandwidth[] = {
        80.0f,  90.0f,  100.0f, 110.0f, 120.0f, 130.0f, 140.0f, 150.0f,
        160.0f, 170.0f, 180.0f, 190.0f, 200.0f, 210.0f, 220.0f, 230.0f,
        240.0f, 250.0f, 260.0f, 270.0f, 280.0f, 290.0f, 300.0f, 310.0f
    };

    const int numBarkBands = sizeof(barkCenterFreq) / sizeof(float);

    float referenceLoudness = 0.0f;  // Loudness of flat frequency response
    float filterLoudness = 0.0f;     // Loudness of current filter response

    // Calculate loudness for each Bark band
    for (int band = 0; band < numBarkBands; ++band)
    {
        float centerFreq = barkCenterFreq[band];
        float bandwidth = barkBandwidth[band];

        // Get magnitude response at Bark band center frequency
        // calculateH returns |H(f)|^(stages+1), so we need to convert to linear magnitude
        float magnitudeResponse = AnalogFilter::calculateH(centerFreq, fs, c, d, stages);
        float linearMagnitude = 0.0f;

        if (stages == 0) {
            // stages=0: calculateH returns |H(f)| directly
            linearMagnitude = sqrtf(magnitudeResponse);
        } else {
            // stages>=1: calculateH returns |H(f)|^(stages+1)
            // Convert back to linear magnitude: |H(f)| = response^(1/(stages+1))
            linearMagnitude = powf(magnitudeResponse, 0.5f / (stages + 1.0f));
        }

        // Get psychoacoustic weighting for this frequency
        float weighting = calculateBS1770Weighting(centerFreq);

        // Accumulate reference loudness (flat response = magnitude 1.0)
        referenceLoudness += 1.0f * weighting * bandwidth;

        // Accumulate filter loudness (current filter response)
        filterLoudness += linearMagnitude * weighting * bandwidth;
    }

    // Calculate compensation factor
    // This factor will scale the filter to have the same overall loudness as flat response
    float compensationFactor = 1.0f;
    if (filterLoudness > 0.0f) {
        compensationFactor = referenceLoudness / filterLoudness;

        // Apply reasonable limits to prevent extreme gain values
        compensationFactor = fmaxf(0.1f, fminf(10.0f, compensationFactor));
    }

    // Debug output to verify compensation behavior
    //~ printf("Bark Band Loudness Compensation:\n");
    //~ printf("  Reference Loudness: %.6f\n", referenceLoudness);
    //~ printf("  Filter Loudness:    %.6f\n", filterLoudness);
    //~ printf("  Compensation Factor: %.6f\n", compensationFactor);
    //~ printf("  Filter Gain Param:  %.6f\n", gain);

    // Apply compensation to feedforward coefficients only
    // This preserves the filter characteristics while adjusting overall loudness
    c[0] *= compensationFactor;
    c[1] *= compensationFactor;
    c[2] *= compensationFactor;
}

    // return result including gain
    return coeff;
}

void AnalogFilter::setEqualPower(bool loudnessCompEnabled_)
{
    if(loudnessCompEnabled != loudnessCompEnabled_)
    {
        loudnessCompEnabled = loudnessCompEnabled_;
        recompute = true;
    }
}

void AnalogFilter::computefiltercoefs(float freq, float q)
{
    coeff = AnalogFilter::computeCoeff(type, freq, q, stages, gain,
            samplerate_f, order, loudnessCompEnabled);
}


void AnalogFilter::setfreq(float frequency)
{
    if(frequency < 0.1f)
        frequency = 0.1f;
    else if ( frequency > MAX_FREQ )
        frequency = MAX_FREQ;

    float rap = freq / frequency;
    if(rap < 1.0f)
        rap = 1.0f / rap;

    frequency = ceilf(frequency);/* fractional Hz changes are not
                                 * likely to be audible and waste CPU,
                                 * esp since we're already smoothing
                                 * changes, so round it */

    if ( fabsf( frequency - freq ) >= 1.0f )
    {
        /* only perform computation if absolutely necessary */
        freq = frequency;
        recompute = true;
    }
    if (recompute)
        q = newq;

    if (beforeFirstTick) {
        freq_smoothing.reset( freq );
        beforeFirstTick=false;
    }
}

void AnalogFilter::setfreq_and_q(float frequency, float q_)
{
    newq = q_;
    /*
     * Only recompute based on Q change if change is more than 10%
     * from current value (or the old or new Q is 0, which normally
     * won't occur, but better to handle it than potentially
     * fail on division by zero or assert).
     */
    if (q == 0.0 || q_ == 0.0 || ((q > q_ ? q / q_ : q_ / q) > 1.1))
        recompute = true;
    setfreq(frequency);
}

void AnalogFilter::setq(float q_)
{
    newq = q = q_;
    computefiltercoefs(freq,q);
}

void AnalogFilter::settype(int type_)
{
    type = type_;
    computefiltercoefs(freq,q);
}

void AnalogFilter::setgain(float dBgain)
{
    gain = dB2rap(dBgain);
    computefiltercoefs(freq,q);
}

void AnalogFilter::setstages(int stages_)
{
    if(stages_ >= MAX_FILTER_STAGES)
        stages_ = MAX_FILTER_STAGES - 1;
    if(stages_  != stages) {
        stages = stages_;
        cleanup();
        computefiltercoefs(freq,q);
    }
}

inline void AnalogBiquadFilterA(const float coeff[5], float &src, float work[4], float &sumIn, float &sumOut, float window)
{
    sumIn += src*src*window;
    work[3] = src*coeff[0]
        + work[0]*coeff[1]
        + work[1]*coeff[2]
        + work[2]*coeff[3]
        + work[3]*coeff[4];
    work[1] = src;
    src     = work[3];
    sumOut += src*src;
}

inline void AnalogBiquadFilterB(const float coeff[5], float &src, float work[4], float &sumIn, float &sumOut, float window)
{
    sumIn += src*src*window;
    work[2] = src*coeff[0]
        + work[1]*coeff[1]
        + work[0]*coeff[2]
        + work[3]*coeff[3]
        + work[2]*coeff[4];
    work[0] = src;
    src     = work[2];
    sumOut += src*src;
}

void AnalogFilter::filterSample(float& smp)
{

    if (recompute) {
        computefiltercoefs(freq, q);  // freq und q müssen aktuell gesetzt sein
        recompute = false;
    }

    fstage &hist = history[0];

    float y0 = smp * coeff.c[0] + hist.x1 * coeff.c[1]
               + hist.y1 * coeff.d[1];
    hist.y1 = y0;
    hist.x1 = smp;
    smp  = y0;
}

void AnalogFilter::singlefilterout(float *smp, fstage &hist, float f, unsigned int bufsize)
{
    assert((buffersize % 8) == 0);

    if ( recompute )
    {
        computefiltercoefs(f,q);
        recompute = false;
    }

    if(order == 1) {  //First order filter
        for(unsigned int i = 0; i < bufsize; ++i) {
            int windowIndex = (windowPos + i) % windowSize;
            float windowPhase = float(windowIndex) / float(windowSize);
            const float window = 0.5f * (1.0f - cosf(2.0f * M_PI * windowPhase));

            sumIn += smp[i]*smp[i]*window;
            float y0 = smp[i] * coeff.c[0] + hist.x1 * coeff.c[1]
                       + hist.y1 * coeff.d[1];
            hist.y1 = y0;
            hist.x1 = smp[i];
            smp[i]  = y0;
            sumOut += smp[i]*smp[i]*window;

            if (windowIndex==0 && loudnessCompEnabled)
            {
                compensationfactor = sqrt(sumIn/windowSize)/sqrt(sumOut/windowSize);
                printf("compensationfactor: %f\n", compensationfactor);
                sumIn = 0.0f;
                sumOut = 0.0f;
            }
            else compensationfactor = 1.0f;

        }
    } else if(order == 2) {//Second order filter
        const float coeff_[5] = {coeff.c[0], coeff.c[1], coeff.c[2],  coeff.d[1], coeff.d[2]};
        float work[4]  = {hist.x1, hist.x2, hist.y1, hist.y2};
        float window = 0.0f;

        for(unsigned int i = 0; i < bufsize; i+=8) {
            for(int j = 0; j < 8; j++) {
                int windowIndex = (windowPos + i + j) % windowSize;
                window = 0.5f * (1.0f - cosf(2.0f * M_PI * windowIndex / (windowSize - 1)));

                if(j % 2 == 0) {
                    AnalogBiquadFilterA(coeff_, smp[i + j], work, sumIn, sumOut, window);
                } else {
                    AnalogBiquadFilterB(coeff_, smp[i + j], work, sumIn, sumOut, window);
                }

                if (windowIndex == 0 && loudnessCompEnabled) {

                    // poor persons IIR filtering
                    compensationfactor = 0.95f * compensationfactor + 0.05f * sqrt(sumIn/windowSize)/sqrt(sumOut/windowSize);
                    sumIn *= 0.1f;
                    sumOut *= 0.1f;
                }
                else compensationfactor = 1.0f;
            }
        }

        hist.x1 = work[0];
        hist.x2 = work[1];
        hist.y1 = work[2];
        hist.y2 = work[3];
    }

    if ( loudnessCompEnabled && sumOut > bufsize/256)
    {

        for(unsigned int i = 0; i < bufsize; ++i) {
            int windowIndex = (windowPos + i) % windowSize;
            const float t = float(windowIndex) / (windowSize - 1);
            const float interpolatedFactor = compensationfactor_hist + t * (compensationfactor - compensationfactor_hist);
            smp[i] *= (interpolatedFactor);
        }
        compensationfactor_hist = compensationfactor;
    }

    windowPos += bufsize;
}

void AnalogFilter::filterout(float *smp)
{
    float freqbuf[freqbufsize];

    if ( freq_smoothing.apply( freqbuf, freqbufsize, freq ) )
    {
        /* in transition, need to do fine grained interpolation */
        for(int i = 0; i < stages + 1; ++i)
            for(int j = 0; j < freqbufsize; ++j)
            {
                recompute = true;
                singlefilterout(&smp[j*8], history[i], freqbuf[j], 8);
            }
    }
    else
    {
        /* stable state, just use one coeff */
        for(int i = 0; i < stages + 1; ++i)
            singlefilterout(smp, history[i], freq, buffersize);
    }

    for(int i = 0; i < buffersize; ++i)
        smp[i] *= outgain;
}
float AnalogFilter::calculateH(float freq, float fs, const float c[3], const float d[3], int stages)
{
    float fr = freq / fs * PI * 2.0f;
    float x = c[0], y = 0.0f;
    for(int n = 1; n < 3; ++n) {
        x += cosf(n * fr) * c[n];
        y -= sinf(n * fr) * c[n];
    }
    float h = x * x + y * y;
    x = 1.0f;
    y = 0.0f;
    for(int n = 1; n < 3; ++n) {
        x -= cosf(n * fr) * d[n];
        y += sinf(n * fr) * d[n];
    }
    h = h / (x * x + y * y);
    return powf(h, (stages + 1.0f) / 2.0f);
}

float AnalogFilter::H(float freq)
{
    return calculateH(freq, samplerate_f, coeff.c, coeff.d, stages);
}

float AnalogFilter::calculateBS1770Weighting(float freq)
{
    // ITU-R BS.1770 High-frequency shelf filter
    //~ float f2 = freq * freq;

    // RLB Weighting (Revised Low Frequency B-weighting)
    float numerator = 1.0f;
    float denominator = 1.0f + (12194.0f / freq) * (12194.0f / freq);

    float weighting = numerator / denominator;
    return weighting * weighting;
}

}
