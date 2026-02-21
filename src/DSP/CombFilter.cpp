#include <cassert>
#include <cstdio>
#include <cmath>
#include <stdio.h>

#include "../Misc/Allocator.h"
#include "../Misc/Util.h"
#include "CombFilter.h"
#include "AnalogFilter.h"

// theory from `Introduction to Digital Filters with Audio Applications'', by Julius O. Smith III, (September 2007 Edition).
// https://www.dsprelated.com/freebooks/filters/Analysis_Digital_Comb_Filter.html

namespace zyn{

CombFilter::CombFilter(Allocator *alloc, unsigned char Ftype, float Ffreq, float Fq,
    unsigned int srate, int bufsize, unsigned char Plpf_, unsigned char Phpf_)
    :Filter(srate, bufsize), gain(1.0f), q(Fq), type(Ftype), Plpf(127), Phpf(0),
    lpf(nullptr), hpf(nullptr), memory(*alloc)
{
    //worst case: looking back from smps[0] at 25Hz using higher order interpolation
    mem_size = (int)ceilf((float)samplerate/25.0) + buffersize + 2; // 2178 at 48000Hz and 256Samples
    input = (float*)memory.alloc_mem(mem_size*sizeof(float));
    output = (float*)memory.alloc_mem(mem_size*sizeof(float));
    memset(input, 0, mem_size*sizeof(float));
    memset(output, 0, mem_size*sizeof(float));

    setfreq_and_q(Ffreq, q);
    settype(type);
    setlpf(Plpf_);
    sethpf(Phpf_);
}

CombFilter::~CombFilter(void)
{
    memory.dealloc(input);
    memory.dealloc(output);
    memory.dealloc(lpf);
    memory.dealloc(hpf);
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
    int poshi_next = (poshi + 1) % mem_size;
    // linear interpolation between samples
    return smp[poshi] + poslo * (smp[poshi_next]-smp[poshi]);
}

void CombFilter::filterout(float *smp)
{
    int endSize = mem_size - inputIndex;
    if (endSize < buffersize)
    {
        // Copy the part of the samples that fit at the end of the buffer
        memcpy(&input[inputIndex], smp, endSize * sizeof(float));
        // Copy the remaining samples to the beginning of the buffer
        memcpy(&input[0], &smp[endSize], (buffersize - endSize) * sizeof(float));
    }
    else
    {
        // Copy new input samples to the buffer
        memcpy(&input[inputIndex], smp, buffersize * sizeof(float));
    }

    for (int i = 0; i < buffersize; i++)
    {
        // Calculate the feedback sample positions in the input buffer
        const float inputPos = fmodf(inputIndex + i - delay + mem_size, mem_size);
        // Calculate the feedback sample positions in the output buffer
        const float outputPos = fmodf(outputIndex - delay + mem_size, mem_size);

        // Feedback berechnen
        float feedback = tanhX(
            gainfwd * sampleLerp(input, inputPos) -
            gainbwd * sampleLerp(output, outputPos));

        // Optional: Feedback filtern
        if (lpf) lpf->filterSample(feedback);
        if (hpf) hpf->filterSample(feedback);

        smp[i] = smp[i] * gain + feedback;

        // Output in Ringpuffer schreiben
        output[outputIndex] = smp[i];

        // Output-Gain anwenden
        smp[i] *= outgain;

        // increase ringbuffer index with turnaround
        outputIndex = (outputIndex + 1) % mem_size;

    }

    //update inputIndex
    inputIndex = (inputIndex + buffersize) % mem_size;

}

void CombFilter::setfreq_and_q(float freq, float q)
{
    setfreq(freq);
    setq(q);
}

void CombFilter::setfreq(float freq)
{
    float ff = limit(freq, 25.0f, 40000.0f);
    delay = ((float)samplerate)/ff - lpfDelay;
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
        case 3:
            gainfwd = 0.0f;
            gainbwd = -q;
            break;
        case 4:
            gainfwd = -q;
            gainbwd = 0.0f;
            break;
        case 5:
            gainfwd = -q;
            gainbwd = -q;
            break;
    }
}

void CombFilter::sethpf(unsigned char _Phpf)
{
    if(Phpf == _Phpf) return;
    Phpf = _Phpf;
    if(Phpf == 0) { //No HighPass
        memory.dealloc(hpf);
    } else {
        const float fr = expf(sqrtf(Phpf / 127.0f) * logf(10000.0f)) + 20.0f;
        if(hpf == NULL)
            hpf = memory.alloc<AnalogFilter>(1, fr, 1, 0, samplerate, buffersize);
        else
            hpf->setfreq(fr);
    }
}

void CombFilter::setlpf(unsigned char _Plpf)
{
    if(Plpf == _Plpf) return;
    Plpf = _Plpf;
    if(Plpf == 127) { //No LowPass
        memory.dealloc(lpf);
        lpfDelay = 0.0f;
    } else {
        const float fr = expf(sqrtf(Plpf / 127.0f) * logf(25000.0f)) + 40.0f;
        if(!lpf)
            lpf = memory.alloc<AnalogFilter>(0, fr, 1, 0, samplerate, buffersize);
        else
            lpf->setfreq(fr);
        const float a = expf(-2.0f * PI * fr / samplerate);
        lpfDelay = a / (1.0f - a);
    }
}

};
