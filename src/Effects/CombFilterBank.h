#include "../Misc/Allocator.h"
#include "../globals.h"
#include "../DSP/Value_Smoothing_Filter.h"

#pragma once

#define MIN_COMB_FREQ 200.0f

namespace zyn {

/**Comb Filter Bank for sympathetic Resonance*/
class CombFilterBank
{
    public:
    CombFilterBank(Allocator *alloc, unsigned int samplerate_, int buffersize_);
    ~CombFilterBank();
    void filterout(float *smp);
   
    float freqs[NUM_SYMPATHETIC_STRINGS]={};
    float inputgain;
    float outgain;
    float gainbwd=0.0f;
    float crossgain=0.0f;
    
    void setStrings(unsigned int nr, float basefreq);
  
    
    private:
    float tanhX(const float x);
    float sampleLerp(float *smp, float pos);
    
    float* output[NUM_SYMPATHETIC_STRINGS] = {};
    float baseFreq;
    unsigned int nrOfStrings=0;
    
    /* for smoothing gain jump when using binary valued sustain pedal */
    Value_Smoothing_Filter gain_smoothing; 
    
    Allocator &memory;
    unsigned int mem_size=0;
    int samplerate=0;
    unsigned int buffersize=0;
    
    
};

}
