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
    unsigned int srate, int bufsize, float tRef_, AbsTime *time_)
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
    phase_offset_old(0.0f),
    phase_offset_fade(0.0f),
    fade_counter(0),
    time(time_),
    memory(*alloc)
{
    samplerate = srate;
    buffersize = bufsize;
    fading_samples = (int)(float(srate)*crossfade),
    max_delay = srate * MAX_REV_DELAY_SECONDS;
    mem_size = (int)(ceilf(max_delay*4.0f)) + 1.27f * samplerate + 2;
    input = (float*)memory.alloc_mem((mem_size+buffersize)*sizeof(float));
    reset();

    setdelay(delay);
    global_offset = fmodf(tRef, delay);
    
    pos_writer = 0;
    pos_start = 0;
    
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
    // copy new input samples to the right end of the buffer
    memcpy(&input[pos_writer], smp, buffersize*sizeof(float));
    const int copysamples = pos_writer - mem_size + buffersize;
    if (copysamples > 0) memcpy(&input[0], &input[mem_size], copysamples*sizeof(float));
    float phase_offset;
    for (int i = 0; i < buffersize; i ++)
    {
        reverse_index++; 
        
        phase_offset = phase_offset_old + (float(i) * phase_offset_fade);
        
        // switch to next buffer
        if((syncMode == AUTO && reverse_index>delay) 
        || (syncMode == HOST && doSync && reverse_index >= syncPos) 
        || (reverse_index >= max_delay && state==PLAYING) // just in case
        )
        {
            doSync = false;
            state = PLAYING;
            printf("XXX\n");
            reverse_index = 0;
            
            // position to start the reverse playing from
            pos_start = pos_writer + buffersize + delay/2.0f;   
            
            const float pos_next = fmodf(float(pos_start+mem_size) - (reverse_index + phase_offset), mem_size);
            delta_crossfade = pos_reader - 1.0 - pos_next;
                    
            fade_counter = 0;  // reset fade counter
        }
        
        if(syncMode == NOTEON && reverse_index > delay && state!=IDLE){
            
            reverse_index = 0;
            fade_counter = 0;  // reset fade counter
            
            if (state==RECORDING) {
                state = PLAYING;
                printf("syncMode: NOTEON state: PLAYING  delay: %f\n", delay);
                // position to start the reverse playing from
                pos_start = pos_writer + buffersize + delay/2.0f;   
            } else if (state==PLAYING) {
                state = IDLE;
                const float pos_next = fmodf(float(pos_start+mem_size) - (reverse_index + phase_offset), mem_size);
                delta_crossfade = pos_reader - 1.0 - pos_next;
                printf("syncMode: NOTEON state: IDLE\n");
            }
        }
                
        // pos_reader
        pos_reader = fmodf(float(pos_start+mem_size) - (reverse_index + phase_offset), mem_size);
        
        if(fade_counter < fading_samples) // inside cross fading segment
        {
            const float slope = float(fade_counter)/float(fading_samples);  // 0 -> 1
            const float fadein = hanningWindow(slope);
            const float fadeout = 1.0f - fadein;      // 1 -> 0
            fade_counter++;

            assert(pos_reader<mem_size && pos_reader >= 0.0f);
            
            // fadeing after switching segments
            if(state == IDLE) // only fade out
                smp[i] = fadeout*sampleLerp(input, fmodf(pos_reader + mem_size + delta_crossfade, mem_size));
            if(state == PLAYING) {
                if (syncMode == NOTEON) // only fade in
                    smp[i] = fadein*sampleLerp(input, pos_reader);
                else // cross fade
                    smp[i] = fadein*sampleLerp(input, pos_reader) + fadeout*sampleLerp(input, fmodf(pos_reader + mem_size + delta_crossfade, mem_size));
            }
        }
        else { // outside cross fading segment
            if(state == PLAYING)
                smp[i] = sampleLerp(input, pos_reader);
        }
        
        // apply output gain
        smp[i] *= gain;
    }
    
    phase_offset_old = phase_offset;
    phase_offset_fade = 0.0f;
    
    // increment writing position
    pos_writer += buffersize;
    pos_writer %= mem_size;
    
}

void Reverter::sync(float pos)
{  
    if(state==IDLE) {
        state = RECORDING;
        reverse_index = 0;
        printf("syncing to noteon state: RECORDING\n");
    }
    else {
        syncPos = pos+reverse_index;
        doSync = true;
    }
    
    
}

void inline Reverter::update_phase(float value)
{
    // update phase_offset
    const float phase_offset_new = value * delay;
    phase_offset_fade = (phase_offset_new - phase_offset_old) / (float)buffersize;    
}


void Reverter::setdelay(float value)
{
    delay = value*float(samplerate);
    
    // limit fading_samples to be < 1/2 delay length
    fading_samples = int(crossfade * float(samplerate));
    if (delay < 2.0f * float(fading_samples))
        fading_samples = int(delay*0.5f);
    
    global_offset = fmodf(tRef, delay);
        
    update_phase(phase);
}

void Reverter::setphase(float value)
{
    phase = value;
    update_phase(phase);
    
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
    if (value != syncMode)
    {
        syncMode = value;
        
        switch(syncMode)
        {
            case NOTEON:
                state = IDLE;
                break;
            default:
                state = PLAYING;
                break;
        }
        printf("setting syncMode: %d state: %d\n", syncMode, state);
        
    }
}

void Reverter::reset()
{
    memset(input, 0, mem_size*sizeof(float));
}

};
