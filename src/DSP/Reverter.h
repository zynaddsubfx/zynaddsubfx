/*
  ZynAddSubFX - a software synthesizer

  Reverter.h - Several analog filters 
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


class Reverter
{
    public:
        //! @param Fq resonance, range [0.1,1000], logscale
        Reverter(Allocator *alloc, float delay,
                unsigned int srate, int bufsize, float tRef=0.0f);
        ~Reverter();
        void filterout(float *smp);
        void setdelay(float delay);
        void setphase(float phase);
        void setgain(float dBgain);
        void reset();

    private:
    
        float* input;
        float gain;

        float step(float x);
        float tanhX(const float x);
        float sampleLerp(float *smp, float pos);
        
        float delay;  
        float phase;
           
        int buffercounter;      
        float reverse_offset;
        float phase_offset;
        float reverse_pos_hist;
        int fading_samples;  
        int fade_counter; 
        
        Allocator &memory;
        int mem_size;
        int samplerate;
        int buffersize;
        float max_delay;

};

}
