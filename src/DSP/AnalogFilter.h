/*
  ZynAddSubFX - a software synthesizer

  Analog Filter.h - Several analog filters (lowpass, highpass...)
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Copyright (C) 2010-2010 Mark McCurry
  Author: Nasca Octavian Paul
          Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef ANALOG_FILTER_H
#define ANALOG_FILTER_H

#include "../globals.h"
#include "Filter.h"
#include "Value_Smoothing_Filter.h"

namespace zyn {

/**Implementation of Several analog filters (lowpass, highpass...)
 * Implemented with IIR filters
 * Coefficients generated with "Cookbook formulae for audio EQ"*/
class AnalogFilter:public Filter
{
    public:
        AnalogFilter(unsigned char Ftype, float Ffreq, float Fq,
                     unsigned char Fstages, unsigned int srate, int bufsize);
        ~AnalogFilter();
        void filterout(float *smp);
        void setfreq(float frequency);
        void setfreq_and_q(float frequency, float q_);
        void setq(float q_);

        void settype(int type_);
        void setgain(float dBgain);
        void setstages(int stages_);
        void cleanup();

        float H(float freq); //Obtains the response for a given frequency


        struct Coeff {
            float c[3], //Feed Forward
                  d[3];    //Feed Back
        } coeff, oldCoeff;

        static Coeff computeCoeff(int type, float cutoff, float q, int stages,
                float gain, float fs, int &order);

    private:
        struct fstage {
            float x1, x2; //Input History
            float y1, y2; //Output History
        } history[MAX_FILTER_STAGES + 1], oldHistory[MAX_FILTER_STAGES + 1];

        //old coeffs are used for interpolation when parameters change quickly

        //Apply IIR filter to Samples, with coefficients, and past history
    void singlefilterout(float *smp, fstage &hist, float f, unsigned int bufsize);// const Coeff &coeff);
        //Update coeff and order
    void computefiltercoefs(float freq, float q);

        int   type;   //The type of the filter (LPF1,HPF1,LPF2,HPF2...)
        int   stages; //how many times the filter is applied (0->1,1->2,etc.)
        float freq;   //Frequency given in Hz
        float q;      //Q factor (resonance or Q factor)
        float gain;   //the gain of the filter (if are shelf/peak) filters
        bool recompute; // need to recompute coeff.
        int order; //the order of the filter (number of poles)

        int freqbufsize;
        Value_Smoothing_Filter freq_smoothing; /* for smoothing freq modulations to avoid zipper effect */
        bool beforeFirstTick; // reset the smoothing at first Tick
};

}

#endif
