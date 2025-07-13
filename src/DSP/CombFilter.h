/*
  ZynAddSubFX - a software synthesizer

  CombFilter.h - Several analog filters
  Copyright (C) 2021-2021 Michael Kirchner
  Author: Michael Kirchner

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#pragma once
#include "Filter.h"
#include "Value_Smoothing_Filter.h"

namespace zyn {

class CombFilter:public Filter
{
    public:
        //! @param Fq resonance, range [0.1,1000], logscale
        CombFilter(Allocator *alloc, unsigned char Ftype, float Ffreq, float Fq,
                unsigned int srate, int bufsize);
        CombFilter(Allocator *alloc, unsigned char Ftype, float Ffreq, float Fq,
                unsigned int srate, int bufsize, unsigned char Plpf_, unsigned char Phpf_);
        ~CombFilter() override;
        void filterout(float *smp) override;
        void setfreq(float freq) override;
        void setfreq_and_q(float freq, float q_) override;
        void setq(float q) override;
        void setgain(float dBgain) override;
        void settype(unsigned char type);
        void sethpf(unsigned char _Phpf);
        void setlpf(unsigned char _Plpf);

    private:

        float* input;
        float* output;
        float gain=1.0f;
        float q;
        unsigned char type;

        unsigned char Plpf;
        unsigned char Phpf;

        float step(float x);

        float tanhX(const float x);
        float sampleLerp(float *smp, float pos);


        float gainfwd;
        float gainbwd;
        float delay;

        class AnalogFilter *lpf, *hpf; //filters

        unsigned int inputIndex=0;
        unsigned int outputIndex=0;

        Allocator &memory;
        int mem_size;

};

}
