#include <cassert>
#include <cstdio>
#include <cmath>
#include <stdio.h>

#include "../Misc/Allocator.h"
#include "../Misc/Util.h"
#include "Reverter.h"

// theory from `Introduction to Digital Filters with Audio Applications'', by Julius O. Smith III, (September 2007 Edition).
// https://www.dsprelated.com/freebooks/filters/Analysis_Digital_Comb_Filter.html

namespace zyn{

Reverter::Reverter(Allocator *alloc, float Ffreq,
    unsigned int srate, int bufsize, float tRef)
    :gain(1.0f), samplerate(srate), buffersize(bufsize), buffercounter(0), fading_samples((int)srate/25), memory(*alloc)
{
    
    // (mem_size-1-buffersize)-(maxdelay-1)-2*fading_samples > 0 --> mem_size = maxdelay + 2*fading_samples + buffersize
    mem_size = (int)ceilf((float)srate*1.50f) + 2*fading_samples + buffersize + 1; // 40bpm -> 1.5s
    
    input = (float*)memory.alloc_mem(mem_size*sizeof(float));
    reset();

    setfreq(Ffreq);
    reverse_offset = fmodf(tRef, delay);
    reverse_pos_hist = -1;
}

Reverter::~Reverter(void)
{
    memory.dealloc(input);
}


inline float Reverter::tanhX(const float x)
{
    // Pade approximation of tanh(x) bound to [-1 .. +1]
    // https://mathr.co.uk/blog/2017-09-06_approximating_hyperbolic_tangent.html
    const float x2 = x*x;
    return (x*(105.0f+10.0f*x2)/(105.0f+(45.0f+x2)*x2)); //
}

inline float Reverter::sampleLerp(float *smp, float pos) {
    int poshi = (int)pos; // integer part (pos >= 0)
    float poslo = pos - (float) poshi; // decimal part
    // linear interpolation between samples
    return smp[poshi] + poslo * (smp[poshi+1]-smp[poshi]); 
}

void Reverter::filterout(float *smp)
{
    // shift the buffer content to the left
    memmove(&input[0], &input[buffersize], (mem_size-buffersize)*sizeof(float));
    // copy new input samples to the right end of the buffer
    memcpy(&input[mem_size-buffersize], smp, buffersize*sizeof(float));
    
    for (int i = 0; i < buffersize; i ++)
    {
        // calculate the current relative position inside the "reverted" buffer
        const float reverse_pos = fmodf((reverse_offset+phase_offset+i),delay);
        // reading head starts at the end of the last buffer and goes backwards
        const float pos = (mem_size-1-buffersize)-reverse_pos; // 
        assert(pos>0);
        
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
            assert(pos-delay>0);
        }
        else { // outside fading segment
            smp[i] = sampleLerp( input, pos);
        }
        
        // apply output gain
        smp[i] *= gain;
    }
    // increase the offset
    reverse_offset += 2*buffersize; // + 1*buffersize because of the leftshifting 
                                    // + 1*buffersize because i turns from buffersize-1 to 0
    // prevent overflow - no effect on reverse_pos in the next cycle
    if (reverse_offset > delay) reverse_offset = fmodf(reverse_offset, delay);
}

void Reverter::setfreq(float freq)
{
    // for reversed delay [0.05 .. 1.5] sec ff= 1/delay 
    float ff = limit(freq, 0.66927f, 20.0f);
    delay = ((float)samplerate)/ff;
    // limit fading_samples to be < 1/2 delay length
    if (int(delay)/2 < int(samplerate/25.0f) ) fading_samples = int(delay)/2;
    else fading_samples = samplerate/25.0f;
    
}

void Reverter::setphase(float phase)
{
    float phase_offset_new = (phase-0.5)*delay;
    if(phase_offset_new != phase_offset) {
        reverse_offset += phase_offset_new - phase_offset;
        phase_offset = phase_offset_new;
    }
}

void Reverter::setgain(float dBgain)
{
    gain = dB2rap(dBgain);
}

void Reverter::reset()
{
    memset(input, 0, mem_size*sizeof(float));
}

};
