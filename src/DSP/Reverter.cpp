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
    unsigned int srate, int bufsize, float tRef_)
    :syncMode(AUTO),
    input(nullptr),
    gain(1.0f),
    delay(delay_),
    phase(0.0f),
    crossfade(0.16f),
    tRef(tRef_),
    buffer_offset(0),
    buffer_counter(0),
    reverse_index(0.0f),
    phase_offset(0.0f),
    fade_counter(0),
    memory(*alloc)
{
    samplerate = srate;
    buffersize = bufsize;
    fading_samples = (int)(float(srate)*crossfade),
    max_delay = srate * MAX_REV_DELAY_SECONDS;
    mem_size = (int)(ceilf(max_delay*4.0f)) + 1.27f * samplerate + 2;
    input = (float*)memory.alloc_mem(mem_size*sizeof(float));
    reset();

    setdelay(delay);
    global_offset = fmodf(tRef, delay);
    
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
        
        reverse_index = fmodf(reverse_index+1, delay);
        const float phase_samples = fmodf(global_offset+phase_offset, delay);
        // turnaround detection
        if((syncMode == AUTO && reverse_index==0) 
        || (syncMode == HOST && doSync && reverse_index == syncPos) 
        || (mem_size - phase_samples - delay - (buffer_offset + reverse_index))<0.0f 
        ) // prevent oor
        {
            doSync = false;
            reverse_index = 0;
            fade_counter = 0;  // reset fade counter
            buffer_counter = 0; // reset buffer counter
            global_offset -= i - i_hist;
            i_hist = i;
        }
        
        buffer_offset = buffer_counter*buffersize;
        
        // calculate the current relative position inside the "reverted" buffer
        const float reverse_pos = buffer_offset + reverse_index;
        
        // reading head
        const float pos = mem_size - delay - reverse_pos - phase_samples;

        assert(pos >= 0 && pos < mem_size);
        
        if(fade_counter < fading_samples) // inside fading segment
        {
            const float slope = float(fade_counter)/float(fading_samples);  // 0 -> 1
            const float fadein = hanningWindow(slope);
            const float fadeout = 1.0f - fadein;               // 1 -> 0
            fade_counter++;

            assert(pos-delay>=0);

            // fade in the newer sampleblock + fade out the older samples
            smp[i] = fadein*sampleLerp( input, pos) + fadeout*sampleLerp( input, pos - 2*delay);
            
        }
        else { // outside fading segment
            smp[i] = sampleLerp( input, pos);
        }
        
        // apply output gain
        smp[i] *= gain;
    }
    // increase the offset
    buffer_counter++;
}

void Reverter::sync(float pos)
{
    syncPos = pos;
    doSync = true;
}

void Reverter::setdelay(float value)
{
    delay = value*float(samplerate);
    
    // limit fading_samples to be < 1/2 delay length
    fading_samples = int(crossfade * float(samplerate));
    if (delay < 2.0f * float(fading_samples))
        fading_samples = int(delay*0.5f);
    
    // update phase_offset
    phase_offset = phase*delay;
    global_offset = fmodf(tRef, delay);
    
}

void Reverter::setphase(float value)
{
    phase = value;
    // update phase_offset
    phase_offset = phase*delay;
    
}
void Reverter::setcrossfade(float value)
{
    crossfade = value;
    fading_samples = int(crossfade * float(samplerate));
    if (delay < 2.0f * float(fading_samples))
        fading_samples = int(delay*0.5f);

}

void Reverter::setgain(float value)
{
    gain = dB2rap(value);
}

void Reverter::setsyncMode(SyncMode value)
{
    syncMode = value;
}

void Reverter::reset()
{
    memset(input, 0, mem_size*sizeof(float));
}

};
