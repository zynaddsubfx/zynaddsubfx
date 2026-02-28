/*
  ZynAddSubFX - a software synthesizer

  Portamento.cpp - Portamento calculation and management
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "Portamento.h"
#include "../globals.h"

namespace zyn {

Portamento::Portamento(const Controller &ctl,
                       const SYNTH_T &synth,
                       bool is_running_note,
                       float oldfreq_log2,
                       float oldportamentofreq_log2,
                       float newfreq_log2)
{
    init(ctl, synth, is_running_note, oldfreq_log2, oldportamentofreq_log2, newfreq_log2);
}

void Portamento::init(const Controller &ctl,
                      const SYNTH_T &synth,
                      bool is_running_note,
                      float oldfreq_log2,
                      float oldportamentofreq_log2,
                      float newfreq_log2)
{
    active = false;

    if(ctl.portamento.portamento == 0)
        return;

    if(ctl.portamento.automode && !is_running_note)
        return;

    if(oldfreq_log2 == newfreq_log2)
        return;

    float portamentotime = powf(100.0f, ctl.portamento.time / 127.0f) / 50.0f; //portamento time in seconds
    const float deltafreq_log2 = oldportamentofreq_log2 - newfreq_log2;
    const float absdeltaf_log2 = fabsf(deltafreq_log2);
    const float absdeltanotefreq_log2 = fabsf(oldfreq_log2 - newfreq_log2);

    if(ctl.portamento.proportional) {
        const float absdeltaf = powf(2.0f, absdeltaf_log2);

        portamentotime *= powf(absdeltaf
            / (ctl.portamento.propRate / 127.0f * 3 + .05),
              (ctl.portamento.propDepth / 127.0f * 1.6f + .2));
    }

    if((ctl.portamento.updowntimestretch >= 64) && (newfreq_log2 < oldfreq_log2)) {
        if(ctl.portamento.updowntimestretch == 127)
            return;
        portamentotime *= powf(0.1f,
                               (ctl.portamento.updowntimestretch - 64) / 63.0f);
    }
    if((ctl.portamento.updowntimestretch < 64) && (newfreq_log2 > oldfreq_log2)) {
        if(ctl.portamento.updowntimestretch == 0)
            return;
        portamentotime *= powf(0.1f,
                               (64.0f - ctl.portamento.updowntimestretch) / 64.0f);
    }

    const float threshold_log2 = ctl.portamento.pitchthresh / 12.0f;
    if((ctl.portamento.pitchthreshtype == 0) && (absdeltanotefreq_log2 - 0.00001f > threshold_log2))
        return;
    if((ctl.portamento.pitchthreshtype == 1) && (absdeltanotefreq_log2 + 0.00001f < threshold_log2))
        return;

    q = ctl.portamento.glissando / 127.0f;  // 0..1

    x = 0.0f;
    dx = synth.buffersize_f / (portamentotime * synth.samplerate_f);
    origfreqdelta_log2 = deltafreq_log2;

    freqdelta_log2 = deltafreq_log2;
    active = true;
}

void Portamento::update(void)
{
    if(!active)
        return;

    // Original portamento progress (0 to 1 over total time)
    x += dx;
    if(x > 1.0f) {
        x = 1.0f;
        active = false;
    }

    // Parameter: 0 = pure portamento (continuous), 1 = pure glissando (stepped)
    float hold_ratio = q;  // 0..1

    // Calculate number of semitone steps
    float total_semitones = fabsf(origfreqdelta_log2 * 12.0f);
    int num_steps = (int)ceilf(total_semitones);

    if(num_steps > 0 && hold_ratio > 0.001f) {
        // Position within current step (0..1)
        float step_position = x * num_steps;
        int current_step = (int)floorf(step_position);
        float step_progress = step_position - current_step;  // 0..1 within step

        // Glissando curve: hold phase + slide phase
        float effective_progress;

        if(step_progress < hold_ratio) {
            // Hold phase: constant frequency at current semitone
            effective_progress = (float)current_step / num_steps;
        } else {
            // Slide phase: transition to next semitone
            float slide_progress = (step_progress - hold_ratio) / (1.0f - hold_ratio);
            effective_progress = (current_step + slide_progress) / num_steps;
        }

        // Ensure we reach exactly 1.0 at the end
        if(current_step >= num_steps - 1) {
            if(step_progress >= hold_ratio) {
                // Final slide completed
                effective_progress = 1.0f;
            }
        }

        freqdelta_log2 = (1.0f - effective_progress) * origfreqdelta_log2;

    } else {
        // Original portamento (continuous)
        freqdelta_log2 = (1.0f - x) * origfreqdelta_log2;
    }
}

PortamentoRealtime::PortamentoRealtime(void *handle,
                                       Allocator &memory,
                                       std::function<void(PortamentoRealtime *)> cleanup,
                                       const Portamento &portamento)
    :handle(handle), memory(memory), cleanup(cleanup), portamento(portamento)
{
}

PortamentoRealtime::~PortamentoRealtime()
{
    cleanup(this);
}

}
