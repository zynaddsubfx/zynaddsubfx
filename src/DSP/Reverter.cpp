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
#include "../Misc/Time.h"
#include "Reverter.h"

namespace zyn {

Reverter::Reverter(Allocator *alloc, float delay_, unsigned int srate, int bufsize, float tRef_, const AbsTime *time_)
    : syncMode(NOTEON), input(nullptr), gain(1.0f), delay(0.0f), phase(0.0f), crossfade(0.16f),
      tRef(tRef_), reverse_index(0.0f), phase_offset_old(0.0f),
      phase_offset_fade(0.0f), fade_counter(0), mean_abs_value(999.9f), time(time_), memory(*alloc),
      samplerate(srate), buffersize(bufsize), max_delay_samples(srate * MAX_REV_DELAY_SECONDS)
{
    setdelay(delay_);
    pos_writer = 0;
    pos_reader = 0;
    pos_start = 0;
    reverse_index = 0;
    state = PLAYING;
}

Reverter::~Reverter() {
    memory.dealloc(input);
}

inline float Reverter::sampleLerp(const float *smp,const float pos) {
    int poshi = static_cast<int>(pos);
    float poslo = pos - static_cast<float>(poshi);
    return smp[poshi] + poslo * (smp[poshi + 1] - smp[poshi]);
}

inline void Reverter::switchBuffers() {
    reverse_index = 0;
    phase_offset = phase * delay;
    // Reset the reverse index to start fresh for the new reverse playback segment.
    pos_start = pos_writer;

    float pos_next = fmodf(float(pos_start + mem_size) - (reverse_index + phase_offset), mem_size);
    // Determine the position of the next sample (`pos_next`) after switching buffers:
    // - Accounts for the current `reverse_index` (reset to 0 above) and phase offset.
    // - Uses modulo (`fmodf`) to ensure the position wraps correctly within the circular buffer.
    // The formula is the same as for pos_reader in updateReaderPosition()

    delta_crossfade = pos_reader - 1.0f - pos_next;
    // Calculate the sample distance between the current read position (`pos_reader`)
    // and the newly computed `pos_next`:
    // - Subtracting `1.0f` compensates for the fact that `pos_reader` corresponds to a read
    //   position from the previous tick.

    fade_counter = 0;
    // Reset the fade counter to begin a new crossfade for the next segment.
}

void Reverter::filterout(float *smp) {
    writeToRingBuffer(smp);
    processBuffer(smp);
}

void Reverter::writeToRingBuffer(const float *smp) {
    int space_to_end = mem_size - pos_writer;
    if (buffersize <= space_to_end) {
        // No wrap around, copy in one go
        memcpy(&input[pos_writer], smp, buffersize * sizeof(float));
    } else {
        // Wrap around, split into two copies
        memcpy(&input[pos_writer], smp, space_to_end * sizeof(float));
        memcpy(&input[0], smp + space_to_end, (buffersize - space_to_end) * sizeof(float));
    }
    // Update pos_writer after copying new buffer
    pos_writer = (pos_writer + buffersize) % mem_size;
    // calculate mean abs value of new buffer (for silence detection)
    float abs_value = 0.0f;
    for (int i = 0; i < buffersize; i++) {
        abs_value += fabsf(smp[i]);
    }
    mean_abs_value = abs_value / static_cast<float>(buffersize);
}

void Reverter::processBuffer(float *smp) {
    for (int i = 0; i < buffersize; i++) {
        reverse_index++;
        checkSync();
        updateReaderPosition();
        crossfadeSamples(smp, i);
        applyGain(smp[i]);
    }

    phase_offset_old = phase_offset;
    phase_offset_fade = 0.0f;
}

void Reverter::checkSync() {
    float delay_samples = delay * static_cast<float>(samplerate);
    switch (syncMode) {
        case AUTO:
            if (reverse_index >= delay_samples) {
                switchBuffers();
            }
            break;
        case HOST:
            if ( (doSync && (float)reverse_index >= syncPos) || // external sync time OR
                reverse_index >= max_delay_samples ) {   // note is too long buffer ends here
                switchBuffers();
                doSync = false;
            }
            // silence the effect if host transport is stopped
            if(!time->playing)
                state = IDLE;
            else
                if (state != PLAYING)
                {
                    state = PLAYING;
                    reset();
                }

            break;
        case NOTEON:
            if (reverse_index >= delay_samples && state != IDLE)
                handleStateChange();
            break;
        case NOTEONOFF:
            if ( (reverse_index >= recorded_samples && state == PLAYING) || // finished playing OR
                 (reverse_index >= max_delay_samples && state == PLAYING) ||// note is too long buffer ends here OR
                 (mean_abs_value < 0.001f && state == RECORDING) )          // note has decayed
                handleStateChange();
            break;
        default:
            {}
            break;
    }
}

void Reverter::handleStateChange() {
    if (state == RECORDING) {
        recorded_samples = reverse_index;
        state = PLAYING;
    } else if (state == PLAYING) {
        state = IDLE;
    }
    switchBuffers();
}

void Reverter::sync(float pos) {
    if (state == IDLE) {
        reset();
        state = RECORDING;
        reverse_index = 0;
    } else {
        syncPos = pos + reverse_index;
        doSync = true;
    }
}

void Reverter::updateReaderPosition() {
    //                 buffersize
    //                  |<---->|
    // ---+------+------+------+------+------+---
    //    |      |      |      |      |      |
    // ...|  b4  |  b5  |  b6  |  b7  |  b8  |...
    // ---+------+------+------+------+------+---
    //             ^           ^             ^
    //             |           L pos_start   L pos_writer
    //             L pos_reader
    //             <--><------->
    //              |       L reverse_index
    //              L phase_offset
    // //////////////////////////////////////////////////

    // update reading position
    pos_reader = fmodf(float(pos_start + mem_size) - (reverse_index + phase_offset), mem_size);
}

void Reverter::crossfadeSamples(float *smp, int i) {
    if (fade_counter < fading_samples) {
        float fadeinFactor = static_cast<float>(fade_counter) / static_cast<float>(fading_samples);
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

void Reverter::setdelay(float value) {

    if (value == delay)
        return;
    else
        delay = value;

    // update crossfade since it depends on delay
    setcrossfade(crossfade);
    // recalculate global offset using new delay
    global_offset = fmodf(tRef, delay);
    // update mem_size since it depends on delay and crossfade
    update_memsize();
}

void Reverter::update_memsize() {
    // Calculate mem_size for reverse delay effect:
    // - 1 times delay for recording
    // - 1 times delay for reverse playing
    // - 1 times delay for phase changing while playing
    // - Add crossfade duration
    // - Add 2 extra samples as a safety margin for interpolation and circular buffer wrapping.
    // - Add buffersize to prevent a buffer write into reading area
    unsigned int mem_size_new;
    if (syncMode == NOTEON || syncMode == AUTO)
        mem_size_new = static_cast<int>(ceilf(3.0f * delay * static_cast<float>(samplerate))) + fading_samples + 2 + buffersize;
    else
        mem_size_new = static_cast<int>(ceilf(3.0f * max_delay_samples)) + fading_samples + 2 + buffersize;

    if (mem_size_new == mem_size)
        return;
    else
        mem_size = mem_size_new;

    if (input != nullptr) {
        memory.devalloc(input);
    }

    input = memory.valloc<float>(mem_size);
    reset();

}

void Reverter::setcrossfade(float value) {
    crossfade = value;
    float delay_samples = delay * static_cast<float>(samplerate);
    fading_samples = static_cast<int>(crossfade * static_cast<float>(samplerate));
    if (delay_samples < 1.25f * static_cast<float>(fading_samples))
        fading_samples = static_cast<int>(delay_samples * 0.8f);
}

void Reverter::setsyncMode(SyncMode value) {
    if (value != syncMode) {
        syncMode = value;
        // initialize state machine appropriate to the new mode
        state = (syncMode == NOTEON || syncMode == NOTEONOFF) ? IDLE : PLAYING;
        // update mem_size since it depends on syncMode
        update_memsize();
    }
}

void Reverter::reset() {
    if (input != nullptr)
        memset(input, 0, mem_size * sizeof(float));
    pos_writer = 0;
    reverse_index = 0;
    syncPos = mem_size;
}

void Reverter::setphase(float value) {
    phase = value;
}

void Reverter::setgain(float value) {
    gain = dB2rap(value);
}

void Reverter::applyGain(float &sample) {
    sample *= gain;
}

};
