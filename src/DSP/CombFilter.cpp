#include <cassert>
#include <cstdio>
#include <cmath>
#include <stdio.h>

#include "../Misc/Allocator.h"
#include "../Misc/Util.h"
#include "CombFilter.h"

// theory from `Introduction to Digital Filters with Audio Applications'', by Julius O. Smith III, (September 2007 Edition).
// https://www.dsprelated.com/freebooks/filters/Analysis_Digital_Comb_Filter.html

namespace zyn{

CombFilter::CombFilter(Allocator *alloc, unsigned char Ftype, float Ffreq, float Fq,
    unsigned int srate, int bufsize)
    :Filter(srate, bufsize), gain(1.0f), q(Fq), type(Ftype), memory(*alloc)
{
    //worst case: looking back from smps[0] at 25Hz using higher order interpolation
    mem_size = (int)ceilf((float)samplerate/25.0) + buffersize + 2; // 2178 at 48000Hz and 256Samples
    input = (float*)memory.alloc_mem(mem_size*sizeof(float));
    output = (float*)memory.alloc_mem(mem_size*sizeof(float));
    memset(input, 0, mem_size*sizeof(float));
    memset(output, 0, mem_size*sizeof(float));

    setfreq_and_q(Ffreq, q);
    settype(type);
}

CombFilter::~CombFilter(void)
{
    memory.dealloc(input);
    memory.dealloc(output);
}


inline float CombFilter::tanhX(const float x)
{
    // Pade approximation of tanh(x) bound to [-1 .. +1]
    // https://mathr.co.uk/blog/2017-09-06_approximating_hyperbolic_tangent.html
    const float x2 = x*x;
    return (x*(105.0f+10.0f*x2)/(105.0f+(45.0f+x2)*x2)); //
}

inline float CombFilter::sampleLerp(float *smp, float pos) {
    int poshi = (int)pos; // integer part (pos >= 0)
    float poslo = pos - (float) poshi; // decimal part
    // linear interpolation between samples
    return smp[poshi] + poslo * (smp[poshi+1]-smp[poshi]); 
}

void CombFilter::filterout(float *smp)
{
    // shift the buffer content to the left
    memmove(&input[0], &input[buffersize], (mem_size-buffersize)*sizeof(float));
    // copy new input samples to the right end of the buffer
    memcpy(&input[mem_size-buffersize], smp, buffersize*sizeof(float));
    for (int i = 0; i < buffersize; i ++)
    {
        // calculate the feedback sample positions in the buffer
        float pos = float(mem_size-buffersize+i)-delay;
        // add the fwd and bwd feedback samples to current sample
        smp[i] = smp[i]*gain + tanhX(
            gainfwd * sampleLerp( input, pos) - 
            gainbwd * sampleLerp(output, pos)); 
        // copy new sample to output buffer
        output[mem_size-buffersize+i] = smp[i];
        // apply output gain
        smp[i] *= outgain;
    }
    // shift the buffer content one buffersize to the left
    memmove(&output[0], &output[buffersize], (mem_size-buffersize)*sizeof(float));
}

void CombFilter::setfreq_and_q(float freq, float q)
{
    setfreq(freq);
    setq(q);
}

void CombFilter::setfreq(float freq)
{
    float ff = limit(freq, 25.0f, 40000.0f);
    delay = ((float)samplerate)/ff;
}

void CombFilter::setq(float q_)
{
    q = cbrtf(0.0015f*q_);
    settype(type);
}

void CombFilter::setgain(float dBgain)
{
    gain = dB2rap(dBgain);
}

void CombFilter::settype(unsigned char type_)
{
    type = type_;
    switch (type)
    {
        case 0:
        default:
            gainfwd = 0.0f;
            gainbwd = q;
            break;
        case 1:
            gainfwd = q;
            gainbwd = 0.0f;
            break;
        case 2:
            gainfwd = q;
            gainbwd = q;
            break;
    }
}

};
