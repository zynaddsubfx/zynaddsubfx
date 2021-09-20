#include <cassert>
#include <cstdio>
#include <cmath>
#include <stdio.h>

#include "../Misc/Allocator.h"
#include "../Misc/Util.h"
#include "CombFilterBank.h"

namespace zyn {

    CombFilterBank::CombFilterBank(Allocator *alloc, unsigned int samplerate_, int buffersize_, unsigned int n):
    inputgain(1.0f),
    outgain(1.0f),
    memory(*alloc),
    samplerate(samplerate_),
    buffersize(buffersize_)
    {
        //worst case: looking back from smps[0] at 200Hz using higher order interpolation
        mem_size = (int)ceilf((float)samplerate/200.0) + buffersize + 2;
        for(int i = 0; i < NUM_SYMPATHETIC_STRINGS; ++i) 
        {
            output[i] = (float*)memory.alloc_mem(mem_size*sizeof(float));
            memset(output[i], 0, mem_size*sizeof(float));
            gainbwd=0.99;
            freqs[i] = 440.0;
            nrOfStrings = 1;
            
        }
        gain_smoothing.sample_rate(samplerate>>4);
        gain_smoothing.thresh(0.02f); // TBD: 2% jump audible?
        gain_smoothing.cutoff(1.0f);
    }

    CombFilterBank::~CombFilterBank()
    {
        for(int i = 0; i < NUM_SYMPATHETIC_STRINGS; ++i)  memory.dealloc(output[i]);
    }

    inline float CombFilterBank::tanhX(const float x)
    {
        // Pade approximation of tanh(x) bound to [-1 .. +1]
        // https://mathr.co.uk/blog/2017-09-06_approximating_hyperbolic_tangent.html
        const float x2 = x*x;
        return (x*(105.0f+10.0f*x2)/(105.0f+(45.0f+x2)*x2)); //
    }

    inline float CombFilterBank::sampleLerp(float *smp, float pos) {
        int poshi = (int)pos; // integer part (pos >= 0)
        float poslo = pos - (float) poshi; // decimal part
        // linear interpolation between samples
        return smp[poshi] + poslo * (smp[poshi+1]-smp[poshi]); 
    }

    void CombFilterBank::filterout(float *smp)
    {
        

        
        float gainbuf[buffersize];
        if (!gain_smoothing.apply( gainbuf, buffersize, gainbwd ) )
            for (unsigned int i = 0; i < buffersize>>4; i ++) gainbuf[i] = gainbwd;

        for (unsigned int i = 0; i < buffersize; i ++)
        {
            for (int j = 0; j < nrOfStrings; ++j)
            {
                // calculate the feedback sample positions in the buffer
                const float delay = ((float)samplerate)/freqs[j];
                const float pos = float(mem_size-buffersize+i) - delay;
                // sample at that position
                const float sample = sampleLerp(output[j], pos);
                
                // cut the lerp (leads to slightly detuned combs)
                //~ const unsigned int delay = (unsigned int)(((float)samplerate)/freqs[j]);
                //~ const int pos = mem_size-buffersize+i-delay;
                //~ const float sample = output[j][pos];
                // subtract limited sample from input and write result to output buffer
                output[j][mem_size-buffersize+i] = smp[i]*inputgain + tanhX(sample*gainbuf[i>>4]);
            }
            // mix output buffer samples to output sample
            smp[i]=0.0f;
            for (int j = 0; j < nrOfStrings; ++j)
                smp[i] += output[j][mem_size-buffersize+i];
            
            // apply output gain but 
            smp[i] *= outgain/(float)nrOfStrings;
        }
        // shift the buffer content one buffersize to the left
        for(int j = 0; j < nrOfStrings; ++j)
            memmove(&output[j][0], &output[j][buffersize], (mem_size-buffersize)*sizeof(float));
    }
}
