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

#define SYNCMODES   AUTO,\
                    MIDI,\
                    HOST,\
                    NOTEON

        enum SyncMode 
        { 
            SYNCMODES
        };

class Reverter
{
    public:


        SyncMode syncMode;  
    
    
        //! @param Fq resonance, range [0.1,1000], logscale
        Reverter(Allocator *alloc, float delay,
                unsigned int srate, int bufsize, float tRef=0.0f, const AbsTime *time_=nullptr);
        ~Reverter();
        void filterout(float *smp);
        void setdelay(float value);
        void setphase(float value);
        void setcrossfade(float value);
        void setgain(float value);
        void setsyncMode(SyncMode value);
        void reset();
        void sync(float pos);
        

    private:
    
        void inline update_phase(float phase);
        
        float* input;
        float gain;

        float step(float x);
        float tanhX(const float x);
        float sampleLerp(float *smp, float pos);
        
        float delay;   
        float phase;
        float crossfade;
        
        float tRef;  
        int buffer_offset; 
        int buffer_counter;
        float global_offset;
        float reverse_index;
        float phase_offset_old;
        float phase_offset_fade;
        int fading_samples;  
        int fade_counter; 
        
        const AbsTime *time;
        
        Allocator &memory;
        unsigned int mem_size;
        int samplerate;
        int buffersize;
        float max_delay;
             
        unsigned int pos_start;
        float syncPos, pos_reader, delta_crossfade;
        bool doSync;
        
        unsigned int pos_writer = 0;

};

}
