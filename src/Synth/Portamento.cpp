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

    x += dx;
    if(x > 1.0f) {
        x    = 1.0f;
        active = false;
    }
    freqdelta_log2 = (1.0f - x) * origfreqdelta_log2;
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
