#include "../Misc/Allocator.h"
#include "../globals.h"
#include "../DSP/Value_Smoothing_Filter.h"

#pragma once

namespace zyn {

/**Comb Filter Bank for sympathetic Resonance*/
class CombFilterBank
{
    public:
    CombFilterBank(Allocator *alloc, unsigned int samplerate_, int buffersize_, unsigned int nrOfStrings_);
    ~CombFilterBank();
    void filterout(float *smp);
   
    //~ int nrOfStrings;
    float freqs[NUM_SYMPATHETIC_STRINGS];
    float inputgain;
    float outgain;
    float gainbwd;//[nrOfStrings];
    
    unsigned int nrOfStrings;
    
    private:
    float tanhX(const float x);
    float sampleLerp(float *smp, float pos);
    
    float* output[NUM_SYMPATHETIC_STRINGS];
    
    /* for smoothing gain jump when using binary valued sustain pedal */
    Value_Smoothing_Filter gain_smoothing; 
    
    Allocator &memory;
    int mem_size;
    int samplerate;
    unsigned int buffersize;
    
    
};

}
