/*
  ZynAddSubFX - a software synthesizer

  Sallen Key Filter.h - 
  Copyright (C) 2023-2023 Michael Kirchner
  Author: Michael Kirchner

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#pragma once
#include "Filter.h"
#include "VAOnePoleFilter.h"
#include "Value_Smoothing_Filter.h"

namespace zyn {

class SallenKeyFilter : public Filter {
public:
    SallenKeyFilter(unsigned char Ftype, float Ffreq, float Fq, unsigned int srate, int bufsize);
    ~SallenKeyFilter();
    void filterout(float *smp);
    void setfreq(float freq);
    void setq(float q);
    void setfreq_and_q(float frequency, float q);
    void setgain(float gain);
    void settype(unsigned char ftype);

private:
    float step(float input);
    float ic1eq,ic2eq;
    float freq;
    //float q;
    float gain;
    unsigned char ftype;
    unsigned sr;
    
    float input_old;
    
    CVAOnePoleFilter m_LPF1;
    CVAOnePoleFilter m_LPF2;
    CVAOnePoleFilter m_HPF1;

    // fn to update when UI changes
    void updateFilters(float freq);
    void singlefilterout(float *smp, unsigned int bufsize);
    
    float tanhX(const float x) const;

    // variables
    float m_dAlpha0;   // our u scalar value

    // enum needed for child members 
    enum{LPF1,HPF1}; /* one short string for each */
    

    float m_dK;
    float m_dSaturation;
    
    int freqbufsize;
    Value_Smoothing_Filter freq_smoothing; /* for smoothing freq modulations to avoid zipper effect */
    
    
    
};

}  // namespace zyn

