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

Reverter::Reverter(Allocator *alloc, float delay_,
    unsigned int srate, int bufsize, float tRef)
    :input(nullptr),
    gain(1.0f),
    delay(delay_),
    phase(0.0f),
    buffercounter(0),
    reverse_offset(0.0f),
    phase_offset(0.0f),
    reverse_pos_hist(0.0f),
    fading_samples((int)srate/4),
    fade_counter(0),
    memory(*alloc)
{
    samplerate = srate;
    buffersize = bufsize;
    max_delay = srate * MAX_REV_DELAY_SECONDS;
    // (mem_size-1-buffersize)-(maxdelay-1)-2*fading_samples-maxphase > 0 
    //  --> mem_size > maxdelay + maxphase + 2*fading_samples + buffersize
    mem_size = (int)ceilf(max_delay*4.0f + fading_samples) + 1; // TBD: calc real factor instead of 4.0
    input = (float*)memory.alloc_mem(mem_size*sizeof(float));
    reset();

    setdelay(delay);
    reverse_offset = fmodf(tRef, delay);
    reverse_pos_hist = -1;
}

Reverter::~Reverter(void)
{
    memory.dealloc(input);
}



inline float Reverter::sampleLerp(float *smp, float pos) {
    int poshi = (int)pos; // integer part (pos >= 0)
    float poslo = pos - (float) poshi; // decimal part
    // linear interpolation between samples
    return smp[poshi] + poslo * (smp[poshi+1]-smp[poshi]); 
}

inline float hanningWindow(float x) {
    return 0.5f * (1.0f - cos(1.0f * M_PI * x));
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
        const float reverse_pos = fmodf((reverse_offset+i+delay),delay);

        
        // turnaround detection
        if(reverse_pos<reverse_pos_hist) {
            fade_counter = 0;  // reset fade counter
        }
        
        // store reverse_pos for turnaround detection
        reverse_pos_hist = reverse_pos; 
        
        // reading head
        float pos = mem_size - 2.0f * delay + fading_samples - reverse_pos ; //


        // Debugging-Ausgabe
        if (pos > mem_size || pos < 0)
        {
            printf("Invalid position detected!\n");
            printf("mem_size: %d\n", mem_size);
            printf("buffersize: %d\n", buffersize);
           
            printf("max_delay: %f\n", max_delay);
            printf("fading_samples: %d\n", fading_samples);
            printf("delay: %f\n", delay);
            printf("phase_offset: %f\n", phase_offset);
            
            printf("i: %d\n", i);
            printf("reverse_offset: %f\n", reverse_offset);
            printf("reverse_pos: %f\n", reverse_pos);
            
            printf("pos: %f\n", pos);
        }
        assert(pos >= 0 && pos < mem_size);
        
        
        // apply phase offset
        pos -= phase_offset;
        
        
        if(fade_counter <= fading_samples) // inside fading segment
        {
            //~ const float windowValue = hanningWindow(); // 0 -> 1
            
            const float fadein = float(fade_counter)/float(fading_samples);  // 0 -> 1
            const float fadeout = 1.0f - fadein;               // 1 -> 0
            fade_counter++;
            // fade in the newer sampleblock + fade out the older samples
            smp[i] = fadein*sampleLerp( input, pos) + fadeout*sampleLerp( input, pos - 1.0f*delay);
            
            assert(pos-1.0f*delay>0);
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

void Reverter::setdelay(float _delay)
{
    delay = _delay*float(samplerate);
    
    // limit fading_samples to be < 1/3 delay length
    if (delay > samplerate/2 ) fading_samples = samplerate/16;
    else fading_samples = (int)(delay)/4.0f;
    
    // update phase_offset
    const float phase_offset_new = (phase-0.5f)*delay;
    if(phase_offset_new != phase_offset) {
        reverse_offset += phase_offset_new - phase_offset;
        phase_offset = phase_offset_new;
    }
    
}

void Reverter::setphase(float _phase)
{
    phase = _phase;
    // update phase_offset
    const float phase_offset_new = (phase-0.5f)*delay;
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
