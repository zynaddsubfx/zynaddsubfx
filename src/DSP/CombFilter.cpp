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
    unsigned int srate, int bufsize, float tRef)
    :Filter(srate, bufsize), gain(1.0f), q(Fq), f_type(Ftype), buffercounter(0), memory(*alloc)
{
    //worst case: looking back from smps[0] at 25Hz using higher order interpolation
    if (Ftype==3) mem_size = (int)ceilf((float)samplerate*1.51f) + buffersize + 2; // 40bpm -> 1.5s
    else mem_size = (int)ceilf((float)samplerate/25.0) + buffersize + 2; // 2178 at 48000Hz and 256Samples
    
    input = (float*)memory.alloc_mem(mem_size*sizeof(float));
    if (Ftype!=3) output = (float*)memory.alloc_mem(mem_size*sizeof(float)); // not needed for Reverse
    reset();

    fading_samples = (int)samplerate/25;
    setfreq_and_q(Ffreq, q);
    reverse_offset = fmodf(tRef, delay);
}

CombFilter::~CombFilter(void)
{
    memory.dealloc(input);
    if (f_type!=3) memory.dealloc(output);
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
        if (reversed)
        {
            // calculate the current relative position inside the "reverted" buffer
            const float reverse_pos = fmodf((reverse_offset+i),delay);
            // reading head starts at the end of the last buffer and goes backwards
            const float pos = (mem_size-1-buffersize)-reverse_pos;
            
            // crossfade for a few samples whenever reverse_pos restarts
            if(reverse_pos<reverse_pos_hist) {
                fade_counter = 0;  // reset fade counter
            }
            reverse_pos_hist = reverse_pos; // store reverse_pos for turnaround detection
            
            if(fade_counter <= fading_samples) // inside fading segment
            {
                const float fadein = (float)fade_counter++ / (float)fading_samples; // 0 -> 1
                const float fadeout = 1.0f - fadein;               // 1 -> 0
                //fade in the newer sampleblock + fade out the older samples
                smp[i] = fadein*sampleLerp( input, pos) + fadeout*sampleLerp( input, pos-delay);
            }
            else { // outside fading segment
                smp[i] = sampleLerp( input, pos);
            }
        }
        else // not in reverse mode
        {
            // calculate the feedback sample positions in the buffer
            float pos = float(mem_size-buffersize+i)-delay;
            // add the fwd and bwd feedback samples to current sample
            smp[i] = smp[i]*gain + tanhX(
                gainfwd * sampleLerp( input, pos) - 
                gainbwd * sampleLerp(output, pos)); 
            // copy new sample to output buffer
            output[mem_size-buffersize+i] = smp[i];
        }
        // apply output gain
        smp[i] *= outgain;
    }
    // shift the buffer content one buffersize to the left
    if (!reversed) memmove(&output[0], &output[buffersize], (mem_size-buffersize)*sizeof(float));
    // increase the offset
    reverse_offset += 2*buffersize; // + 1*buffersize because of the leftshifting 
                                    // + 1*buffersize because i turns from buffersize-1 to 0
    // prevent overflow
    if (reverse_offset > delay) reverse_offset = fmodf(reverse_offset, delay);
}

void CombFilter::setfreq_and_q(float freq, float q)
{
    setfreq(freq);
    setq(q);
}

void CombFilter::setfreq(float freq)
{
    // for reversed delay [0.05 .. 1.5] sec ff= 1/delay 
    float ff = (reversed ? limit(freq, 0.5f, 20.0f) : limit(freq, 25.0f, 40000.0f));
    delay = ((float)samplerate)/ff;
}

void CombFilter::setphase(float phase)
{
    // for reversed delay [0.05 .. 1.5] sec ff= 1/delay 
    if(phase != phase_offset) {
        reverse_offset += phase - phase_offset;
        phase_offset = phase;
    }
}

void CombFilter::setq(float q_)
{
    q = cbrtf(0.0015f*q_);
    settype(f_type);
}

void CombFilter::setgain(float dBgain)
{
    gain = dB2rap(dBgain);
}

void CombFilter::settype(unsigned char type_)
{
    if (f_type != type_)
    {
        if (type_!=3) // switching to comb
        {
            memory.dealloc(input); // dealloc to change size
            mem_size = (int)ceilf((float)samplerate/25.0) + buffersize + 2;
            input = (float*)memory.alloc_mem(mem_size*sizeof(float));
            output = (float*)memory.alloc_mem(mem_size*sizeof(float));
        }
        else // switching to reverse
        {
            mem_size = (int)ceilf((float)samplerate*1.51f) + buffersize + 2;
            memory.dealloc(input);
            memory.dealloc(output); // not needed anymore
            input = (float*)memory.alloc_mem(mem_size*sizeof(float));
        }
        f_type = type_;
        reset();
    }

    switch (f_type)
    {
        case 0:
        default:
            gainfwd = 0.0f;
            gainbwd = q;
            reversed = false;
            break;
        case 1:
            gainfwd = q;
            gainbwd = 0.0f;
            reversed = false;
            break;
        case 2:
            gainfwd = q;
            gainbwd = q;
            reversed = false;
            break;
        case 3:
            gainfwd = 0.0f;
            gainbwd = 0.0f;
            reversed = true;
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

void CombFilter::reset()
{
    memset(input, 0, mem_size*sizeof(float));
    if (f_type!=3) memset(output, 0, mem_size*sizeof(float));
}

};
