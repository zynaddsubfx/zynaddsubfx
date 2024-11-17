/*
  ZynAddSubFX - a software synthesizer

  Reverter.cpp - Reverse Delay
  Copyright (C) 2023-2024 Michael Kirchner
  Author: Michael Kirchner

  This program is free software; you can redistribute it and/or
  modify it under the teabs_value of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cassert>
#include <cstdio>
#include <cmath>
#include <cstring>

#include "../Misc/Allocator.h"
#include "../Misc/Util.h"
#include "Reverter.h"

namespace zyn {

Reverter::Reverter(Allocator *alloc, float delay_, unsigned int srate, int bufsize, float tRef_, const AbsTime *time_)
    : syncMode(AUTO), input(nullptr), gain(1.0f), delay(delay_), phase(0.0f), crossfade(0.16f),
      tRef(tRef_), buffer_offset(0), buffer_counter(0), reverse_index(0.0f), phase_offset_old(0.0f),
      phase_offset_fade(0.0f), fade_counter(0), mean_abs_value(999.9f), time(time_), memory(*alloc), 
      samplerate(srate), buffersize(bufsize) 
{

    // current number of samples to be used for crossfade
    fading_samples = static_cast<int>(srate * crossfade);

    // maximum number of samples for a reversed segment.
    max_delay = srate * MAX_REV_DELAY_SECONDS;
    // Calculate mem_size for reverse delay effect:
    // - 1 times max_delay for recording
    // - 1 times max_delay for reverse playing
    // - 1 times max_delay for phase changing while playing
    // - Add maximum crossfade duration (1.27s).
    // - Add 2 extra samples as a safety margin for interpolation and circular buffer wrapping.
    mem_size = static_cast<int>(ceilf(max_delay * 3.0f)) + static_cast<int>(1.27f * samplerate) + 2;

    input = static_cast<float *>(memory.alloc_mem(mem_size * sizeof(float)));
    reset();

    setdelay(delay);
    global_offset = fmodf(tRef, delay);
    pos_writer = 0;
    pos_reader = 0;
    pos_start = 0;
    reverse_index = 0;
    state = PLAYING;
}

Reverter::~Reverter() {
    memory.dealloc(input);
}

inline float Reverter::sampleLerp(float *smp, float pos) {
    int poshi = static_cast<int>(pos);
    float poslo = pos - static_cast<float>(poshi);
    return smp[poshi] + poslo * (smp[poshi + 1] - smp[poshi]);
}

inline float hanningWindow(float x) {
    return 0.5f * (1.0f - cos(M_PI * x));
}

inline void Reverter::switchBuffers() {
    reverse_index = 0; 
    // Reset the reverse index to start fresh for the new reverse playback segment.
    
    pos_start = pos_writer;
    // Calculate the starting point for reverse playback:
    // - `pos_writer` is the current write position.
    // - `offset` further adjusts the position to align with delay or phase offsets.

    float pos_next = fmodf(float(pos_start + mem_size) - (reverse_index + phase_offset), mem_size); 
    // Determine the position of the next sample (`pos_next`) after switching buffers:
    // - Accounts for the current `reverse_index` (reset to 0 above) and phase offset.
    // - Uses modulo (`fmodf`) to ensure the position wraps correctly within the circular buffer.
    
    delta_crossfade = pos_reader - 1.0f - pos_next; 
    // Calculate the crossfade offset between the current read position (`pos_reader`)
    // and the newly computed `pos_next`:
    // - Subtracting `1.0f` compensates for the fact that `pos_reader` corresponds to a read
    //   position from the previous tick, ensuring alignment with the updated playback.

    fade_counter = 0; 
    // Reset the fade counter to begin a new crossfade for the next segment.
}

void Reverter::filterout(float *smp) {
    writeToRingBuffer(smp);
    processBuffer(smp);
}

void Reverter::writeToRingBuffer(const float *smp) {
    int space_to_end = mem_size - pos_writer;
    float abs_value = 0.0f;

    if (buffersize <= space_to_end) {
        // No wrap around, copy in one go
        memcpy(&input[pos_writer], smp, buffersize * sizeof(float));
    } else {
        // Wrap around, split into two copies
        memcpy(&input[pos_writer], smp, space_to_end * sizeof(float));
        memcpy(&input[0], smp + space_to_end, (buffersize - space_to_end) * sizeof(float));
    }
    for (int i = 0; i < buffersize; i++) {
        abs_value += fabsf(smp[i]);
    }
    // Update pos_writer after copying new buffer
    pos_writer = (pos_writer + buffersize) % mem_size;
    // calculate mean abs value of new buffer (for silence detection)
    mean_abs_value = abs_value / static_cast<float>(buffersize);
}

void Reverter::processBuffer(float *smp) {
    for (int i = 0; i < buffersize; i++) {
        reverse_index++;
        handleSync();
        updateReaderPosition(i);
        crossfadeSamples(smp, i);
        applyGain(smp[i]);
    }

    phase_offset_old = phase_offset;
    phase_offset_fade = 0.0f;
}

void Reverter::handleSync() {
    switch (syncMode) {
        case AUTO:
            if (reverse_index >= delay) switchBuffers();
            break;
        case HOST:
        case MIDI:
            if (doSync && reverse_index >= syncPos) switchBuffers();
            break;
        case NOTEON:
        case NOTEONOFF:
            handleNoteSync();
            
            break;
    }
    if (reverse_index >= max_delay && state == PLAYING) switchBuffers();
}

void Reverter::handleNoteSync() {
    if (syncMode == NOTEON && reverse_index >= delay && state != IDLE) {
        handleStateChange();
    } else if ( syncMode == NOTEONOFF &&  
                (   
                  (reverse_index >= recorded_samples && state == PLAYING) ||
                  (reverse_index >= max_delay && state == PLAYING) ||
                  (mean_abs_value < 0.001f && state == RECORDING)
                )
              ) {
        handleStateChange();
    }
}

void Reverter::handleStateChange() {
    if (state == RECORDING) {
        recorded_samples = reverse_index;
        state = PLAYING; printf("PLAYING\n");
    } else if (state == PLAYING) {
        state = IDLE; printf("IDLE\n");
    }
    switchBuffers();
}

void Reverter::updateReaderPosition(int i) {
    //                 buffersize
    //                  <----->
    // ---+------+------+------+------+------+---
    //    |      |      |      |      |      |
    // ...|  b4  |  b5  |  b6  |  b7  |  b8  |...
    // ---+------+------+------+------+------+---
    //             ^           ^             ^
    //             |           L pos_start   L pos_writer
    //             L pos_reader
    //             <--><--_---->
    //              |       L reverse_index
    //              L phase_offset   
    //    
    
    // linear interpolate phase
    phase_offset = phase_offset_old + static_cast<float>(i) * phase_offset_fade;
    // update reading position
    pos_reader = fmodf(float(pos_start + mem_size) - (reverse_index + phase_offset), mem_size);
}

void Reverter::crossfadeSamples(float *smp, int i) {
    if (fade_counter < fading_samples) {
        float fadePhase = static_cast<float>(fade_counter) / static_cast<float>(fading_samples);
        //~ float fadeinFactor = hanningWindow(fadePhase);
        float fadeinFactor = fadePhase;
        float fadeoutFactor = 1.0f - fadeinFactor;
        fade_counter++;

        if (state == IDLE) {
            // in IDLE only the fade out part is needed
            smp[i] = fadeoutFactor * sampleLerp(input, fmodf(pos_reader + mem_size + delta_crossfade, mem_size));
        } else {
            smp[i] = applyFade(fadeinFactor, fadeoutFactor);
        }
    } else {
        smp[i] = (state == PLAYING) ? sampleLerp(input, pos_reader) : 0.0f;
    }
}

float Reverter::applyFade(float fadein, float fadeout) {
    if (syncMode == NOTEON || syncMode == NOTEONOFF) {
        return fadein * sampleLerp(input, pos_reader);
    } else {
        return fadein * sampleLerp(input, pos_reader) + fadeout * sampleLerp(input, fmodf(pos_reader + mem_size + delta_crossfade, mem_size));
    }
}

void Reverter::applyGain(float &sample) {
    sample *= gain;
}

void Reverter::sync(float pos) {
    if (state == IDLE) {
        reset();
        state = RECORDING; printf("RECORDING\n");
        reverse_index = 0.0f;
        mean_abs_value = 999.9f;
    } else {
        syncPos = pos + reverse_index;
        doSync = true;
    }
}

void Reverter::update_phase(float value) {
    float phase_offset_new = value * delay;
    phase_offset_fade = (phase_offset_new - phase_offset_old) / static_cast<float>(buffersize);
}

void Reverter::setdelay(float value) {
    delay = value * static_cast<float>(samplerate);
    fading_samples = static_cast<int>(crossfade * static_cast<float>(samplerate));
    if (delay < 2.0f * static_cast<float>(fading_samples)) fading_samples = static_cast<int>(delay * 0.5f);

    global_offset = fmodf(tRef, delay);
    update_phase(phase);
}

void Reverter::setphase(float value) {
    phase = value;
    update_phase(phase);
}

void Reverter::setcrossfade(float value) {
    crossfade = value+1;
    fading_samples = static_cast<int>(crossfade * static_cast<float>(samplerate));
    if (delay < 2.0f * static_cast<float>(fading_samples)) fading_samples = static_cast<int>(delay * 0.5f);
}

void Reverter::setgain(float value) {
    gain = dB2rap(value);
}

void Reverter::setsyncMode(SyncMode value) {
    if (value != syncMode) {
        syncMode = value;
        state = (syncMode == NOTEON || syncMode == NOTEONOFF) ? IDLE : PLAYING;
        printf("setting syncMode: %d state: %s\n", syncMode, state==IDLE ? "IDLE" : "PLAYING");
        reset();
    }
}

void Reverter::reset() {
    memset(input, 0, mem_size * sizeof(float));
}

};
