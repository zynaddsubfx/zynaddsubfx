/*
  ZynAddSubFX - a software synthesizer

  SV Filter.h - Several state-variable filters
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef SV_FILTER_H
#define SV_FILTER_H

#include "../globals.h"
#include "Filter.h"
#include "Value_Smoothing_Filter.h"

namespace zyn {

class SVFilter:public Filter
{
    public:
        SVFilter(unsigned char Ftype,
                 float Ffreq,
                 float Fq,
                 unsigned char Fstages,
                 unsigned int srate, int bufsize);
        ~SVFilter();
        void filterout(float *smp);
        void setfreq(float frequency);
        void setfreq_and_q(float frequency, float q_);
        void setq(float q_);

        void settype(int type_);
        void setgain(float dBgain);
        void setstages(int stages_);
        void cleanup();

        struct response {
            response(float b0, float b1, float b2,
                     float a0, float a1 ,float a2);
            float a[3];
            float b[3];
        };
        static response computeResponse(int type,
                float freq, float pq, int stages, float g, float fs);

    private:
        struct fstage {
            float low, high, band, notch;
        } st[MAX_FILTER_STAGES + 1];

        struct parameters {
            float f, q, q_sqrt;
        } par, ipar;

        float *getfilteroutfortype(SVFilter::fstage &x);
    void singlefilterout(float *smp, fstage &x, parameters &par, int buffersize );

    void computefiltercoefs(void);
        int   type;    // The type of the filter (LPF1,HPF1,LPF2,HPF2...)
        int   stages;  // how many times the filter is applied (0->1,1->2,etc.)
        float freq; // Frequency given in Hz
        float q;    // Q factor (resonance or Q factor)
        float gain; // the gain of the filter (if are shelf/peak) filters

    Value_Smoothing_Filter freq_smoothing;
};

}

#endif
