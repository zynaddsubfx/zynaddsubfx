/*
  ZynAddSubFX - a software synthesizer

  pADnote.cpp - The "pad" synthesizer
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include <cassert>
#include <cmath>
#include "PADnote.h"
#include "ModFilter.h"
#include "../Misc/Config.h"
#include "../Misc/Allocator.h"
#include "../Params/PADnoteParameters.h"
#include "../Params/Controller.h"
#include "../Params/FilterParams.h"
#include "../Containers/ScratchString.h"
#include "../Containers/NotePool.h"
#include "../Misc/Util.h"

namespace zyn {

PADnote::PADnote(const PADnoteParameters *parameters,
                 const SynthParams &pars, const int& interpolation, WatchManager *wm,
                 const char *prefix)
    :SynthNote(pars),
    watch_int(wm, prefix, "noteout/after_interpolation"), watch_punch(wm, prefix, "noteout/after_punch"),
    watch_amp_int(wm, prefix, "noteout/after_amp_interpolation"), watch_legato(wm, prefix, "noteout/after_legato"),
     pars(*parameters),interpolation(interpolation)
{
    NoteGlobalPar.GlobalFilter    = nullptr;
    NoteGlobalPar.FilterEnvelope  = nullptr;
    NoteGlobalPar.FilterLfo       = nullptr;

    firsttime = true;
    setup(pars.velocity, pars.portamento, pars.note_log2_freq, false, wm, prefix);
}

void PADnote::setup(float velocity_,
                    int portamento_,
                    float note_log2_freq_,
                    bool legato,
                    WatchManager *wm,
                    const char *prefix)
{
    portamento = portamento_;
    velocity   = velocity_;
    finished_  = false;

    if(pars.Pfixedfreq == 0) {
        note_log2_freq = note_log2_freq_;
    }
    else { //the fixed freq is enabled
        const int fixedfreqET = pars.PfixedfreqET;
        float fixedfreq_log2 = log2f(440.0f);

        if(fixedfreqET != 0) { //if the frequency varies according the keyboard note
            float tmp_log2 = (note_log2_freq_ - fixedfreq_log2) *
                (powf(2.0f, (fixedfreqET - 1) / 63.0f) - 1.0f);
            if(fixedfreqET <= 64)
                fixedfreq_log2 += tmp_log2;
            else
                fixedfreq_log2 += tmp_log2 * log2f(3.0f);
        }
        note_log2_freq = fixedfreq_log2;
    }
    const float basefreq = powf(2.0f, note_log2_freq);

    int BendAdj = pars.PBendAdjust - 64;
    if (BendAdj % 24 == 0)
        BendAdjust = BendAdj / 24;
    else
        BendAdjust = BendAdj / 24.0f;
    float offset_val = (pars.POffsetHz - 64)/64.0f;
    OffsetHz = 15.0f*(offset_val * sqrtf(fabsf(offset_val)));
    if(!legato) firsttime = true;
    realfreq  = basefreq;
    if(!legato)
        NoteGlobalPar.Detune = getdetune(pars.PDetuneType, pars.PCoarseDetune,
                                         pars.PDetune);


    //find out the closest note
    const float log2freq = note_log2_freq + NoteGlobalPar.Detune / 1200.0f;
    float mindist = fabsf(log2freq - log2f(pars.sample[0].basefreq + 0.0001f));
    nsample = 0;
    for(int i = 1; i < PAD_MAX_SAMPLES; ++i) {
        if(pars.sample[i].smp == NULL)
            break;
        const float dist = fabsf(log2freq - log2f(pars.sample[i].basefreq + 0.0001f));

        if(dist < mindist) {
            nsample = i;
            mindist = dist;
        }
    }

    int size = pars.sample[nsample].size;
    if(size == 0)
        size = 1;


    if(!legato) { //not sure
        poshi_l = (int)(RND * (size - 1));
        if(pars.PStereo)
            poshi_r = (poshi_l + size / 2) % size;
        else
            poshi_r = poshi_l;
        poslo = 0.0f;
    }


    if(pars.PPanning)
        NoteGlobalPar.Panning = pars.PPanning / 128.0f;
    else if(!legato)
        NoteGlobalPar.Panning = RND;

    if(!legato) {
        NoteGlobalPar.Fadein_adjustment =
            pars.Fadein_adjustment / (float)FADEIN_ADJUSTMENT_SCALE;
        NoteGlobalPar.Fadein_adjustment *= NoteGlobalPar.Fadein_adjustment;
        if(pars.PPunchStrength != 0) {
            NoteGlobalPar.Punch.Enabled = 1;
            NoteGlobalPar.Punch.t = 1.0f; //start from 1.0f and to 0.0f
            NoteGlobalPar.Punch.initialvalue =
                ((powf(10, 1.5f * pars.PPunchStrength / 127.0f) - 1.0f)
                 * VelF(velocity,
                        pars.PPunchVelocitySensing));
            const float time =
                powf(10, 3.0f * pars.PPunchTime / 127.0f) / 10000.0f;             //0.1f .. 100 ms
            const float freq = powf(2.0f, note_log2_freq_);
            const float stretch = powf(440.0f / freq, pars.PPunchStretch / 64.0f);
            NoteGlobalPar.Punch.dt = 1.0f
                                     / (time * synth.samplerate_f * stretch);
        }
        else
            NoteGlobalPar.Punch.Enabled = 0;

        ScratchString pre = prefix;

        NoteGlobalPar.FreqEnvelope =
            memory.alloc<Envelope>(*pars.FreqEnvelope, basefreq, synth.dt(),
                    wm, (pre+"FreqEnvelope/").c_str);
        NoteGlobalPar.FreqLfo      =
            memory.alloc<LFO>(*pars.FreqLfo, basefreq, time,
                    wm, (pre+"FreqLfo/").c_str);

        NoteGlobalPar.AmpEnvelope =
            memory.alloc<Envelope>(*pars.AmpEnvelope, basefreq, synth.dt(),
                    wm, (pre+"AmpEnvelope/").c_str);
        NoteGlobalPar.AmpLfo      =
            memory.alloc<LFO>(*pars.AmpLfo, basefreq, time,
                    wm, (pre+"AmpLfo/").c_str);
    }

    NoteGlobalPar.Volume = 4.0f
                           * powf(0.1f, 3.0f * (1.0f - pars.PVolume / 96.0f))      //-60 dB .. 0 dB
                           * VelF(velocity, pars.PAmpVelocityScaleFunction); //velocity sensing

    if (!legato) {
        NoteGlobalPar.AmpEnvelope->envout_dB(); //discard the first envelope output
        globaloldamplitude = globalnewamplitude = NoteGlobalPar.Volume
                                                  * NoteGlobalPar.AmpEnvelope->
                                                  envout_dB()
                                                  * NoteGlobalPar.AmpLfo->amplfoout();
    }

    if(!legato) {
        ScratchString pre = prefix;
        auto &flt = NoteGlobalPar.GlobalFilter;
        auto &env = NoteGlobalPar.FilterEnvelope;
        auto &lfo = NoteGlobalPar.FilterLfo;
        assert(flt == nullptr);
        flt = memory.alloc<ModFilter>(*pars.GlobalFilter, synth, time, memory, true, basefreq);

        //setup mod
        env = memory.alloc<Envelope>(*pars.FilterEnvelope, basefreq,
                synth.dt(), wm, (pre+"FilterEnvelope/").c_str);
        lfo = memory.alloc<LFO>(*pars.FilterLfo, basefreq, time,
                wm, (pre+"FilterLfo/").c_str);
        flt->addMod(*env);
        flt->addMod(*lfo);
    }

    {
        auto &flt = *NoteGlobalPar.GlobalFilter;
        flt.updateSense(velocity, pars.PFilterVelocityScale,
                        pars.PFilterVelocityScaleFunction);
        flt.updateNoteFreq(basefreq);
    }

    if(!pars.sample[nsample].smp) {
        finished_ = true;
        return;
    }
}

SynthNote *PADnote::cloneLegato(void)
{
    SynthParams sp{memory, ctl, synth, time, velocity,
                   (bool)portamento, legato.param.note_log2_freq, true, legato.param.seed};
    return memory.alloc<PADnote>(&pars, sp, interpolation);
}

void PADnote::legatonote(const LegatoParams &pars)
{
    // Manage legato stuff
    if(legato.update(pars))
        return;

    setup(pars.velocity, pars.portamento, pars.note_log2_freq, true);
}


PADnote::~PADnote()
{
    memory.dealloc(NoteGlobalPar.FreqEnvelope);
    memory.dealloc(NoteGlobalPar.FreqLfo);
    memory.dealloc(NoteGlobalPar.AmpEnvelope);
    memory.dealloc(NoteGlobalPar.AmpLfo);
    memory.dealloc(NoteGlobalPar.GlobalFilter);
    memory.dealloc(NoteGlobalPar.FilterEnvelope);
    memory.dealloc(NoteGlobalPar.FilterLfo);
}


inline void PADnote::fadein(float *smps)
{
    int zerocrossings = 0;
    for(int i = 1; i < synth.buffersize; ++i)
        if((smps[i - 1] < 0.0f) && (smps[i] > 0.0f))
            zerocrossings++;                                  //this is only the positive crossings

    float tmp = (synth.buffersize_f - 1.0f) / (zerocrossings + 1) / 3.0f;
    if(tmp < 8.0f)
        tmp = 8.0f;
    tmp *= NoteGlobalPar.Fadein_adjustment;

    int n;
    F2I(tmp, n); //how many samples is the fade-in
    if(n > synth.buffersize)
        n = synth.buffersize;
    for(int i = 0; i < n; ++i) { //fade-in
        float tmp = 0.5f - cosf((float)i / (float) n * PI) * 0.5f;
        smps[i] *= tmp;
    }
}


void PADnote::computecurrentparameters()
{
    const float relfreq = getFilterCutoffRelFreq();
    const float globalpitch = 0.01f * (NoteGlobalPar.FreqEnvelope->envout()
                           + NoteGlobalPar.FreqLfo->lfoout()
                           * ctl.modwheel.relmod + NoteGlobalPar.Detune);
    globaloldamplitude = globalnewamplitude;
    globalnewamplitude = NoteGlobalPar.Volume
                         * NoteGlobalPar.AmpEnvelope->envout_dB()
                         * NoteGlobalPar.AmpLfo->amplfoout();

    NoteGlobalPar.GlobalFilter->update(relfreq, ctl.filterq.relq);

    //compute the portamento, if it is used by this note
    float portamentofreqdelta_log2 = 0.0f;
    if(portamento) { //this voice use portamento
        portamentofreqdelta_log2 = ctl.portamento.freqdelta_log2;
        if(ctl.portamento.used == 0) //the portamento has finished
            portamento = false;  //this note is no longer "portamented"
    }

    realfreq =
        powf(2.0f, note_log2_freq + globalpitch / 12.0f + portamentofreqdelta_log2) *
        powf(ctl.pitchwheel.relfreq, BendAdjust) + OffsetHz;
}


int PADnote::Compute_Linear(float *outl,
                            float *outr,
                            int freqhi,
                            float freqlo)
{
    const float *smps = pars.sample[nsample].smp;
    if(smps == NULL) {
        finished_ = true;
        return 1;
    }
    int size = pars.sample[nsample].size;
    for(int i = 0; i < synth.buffersize; ++i) {
        poshi_l += freqhi;
        poshi_r += freqhi;
        poslo   += freqlo;
        if(poslo >= 1.0f) {
            poshi_l += 1;
            poshi_r += 1;
            poslo   -= 1.0f;
        }
        if(poshi_l >= size)
            poshi_l %= size;
        if(poshi_r >= size)
            poshi_r %= size;

        outl[i] = smps[poshi_l] * (1.0f - poslo) + smps[poshi_l + 1] * poslo;
        outr[i] = smps[poshi_r] * (1.0f - poslo) + smps[poshi_r + 1] * poslo;
    }
    return 1;
}
int PADnote::Compute_Cubic(float *outl,
                           float *outr,
                           int freqhi,
                           float freqlo)
{
    float *smps = pars.sample[nsample].smp;
    if(smps == NULL) {
        finished_ = true;
        return 1;
    }
    int   size = pars.sample[nsample].size;
    float xm1, x0, x1, x2, a, b, c;
    for(int i = 0; i < synth.buffersize; ++i) {
        poshi_l += freqhi;
        poshi_r += freqhi;
        poslo   += freqlo;
        if(poslo >= 1.0f) {
            poshi_l += 1;
            poshi_r += 1;
            poslo   -= 1.0f;
        }
        if(poshi_l >= size)
            poshi_l %= size;
        if(poshi_r >= size)
            poshi_r %= size;


        //left
        xm1     = smps[poshi_l];
        x0      = smps[poshi_l + 1];
        x1      = smps[poshi_l + 2];
        x2      = smps[poshi_l + 3];
        a       = (3.0f * (x0 - x1) - xm1 + x2) * 0.5f;
        b       = 2.0f * x1 + xm1 - (5.0f * x0 + x2) * 0.5f;
        c       = (x1 - xm1) * 0.5f;
        outl[i] = (((a * poslo) + b) * poslo + c) * poslo + x0;
        //right
        xm1     = smps[poshi_r];
        x0      = smps[poshi_r + 1];
        x1      = smps[poshi_r + 2];
        x2      = smps[poshi_r + 3];
        a       = (3.0f * (x0 - x1) - xm1 + x2) * 0.5f;
        b       = 2.0f * x1 + xm1 - (5.0f * x0 + x2) * 0.5f;
        c       = (x1 - xm1) * 0.5f;
        outr[i] = (((a * poslo) + b) * poslo + c) * poslo + x0;
    }
    return 1;
}


int PADnote::noteout(float *outl, float *outr)
{
    computecurrentparameters();
    float *smps = pars.sample[nsample].smp;
    if(smps == NULL) {
        for(int i = 0; i < synth.buffersize; ++i) {
            outl[i] = 0.0f;
            outr[i] = 0.0f;
        }
        return 1;
    }
    float smpfreq = pars.sample[nsample].basefreq;


    float freqrap = realfreq / smpfreq;
    int   freqhi  = (int) (floor(freqrap));
    float freqlo  = freqrap - floorf(freqrap);


    if(interpolation)
        Compute_Cubic(outl, outr, freqhi, freqlo);
    else
        Compute_Linear(outl, outr, freqhi, freqlo);

    watch_int(outl,synth.buffersize);

    if(firsttime) {
        fadein(outl);
        fadein(outr);
        firsttime = false;
    }

    NoteGlobalPar.GlobalFilter->filter(outl, outr);

    //Apply the punch
    if(NoteGlobalPar.Punch.Enabled != 0)
        for(int i = 0; i < synth.buffersize; ++i) {
            float punchamp = NoteGlobalPar.Punch.initialvalue
                             * NoteGlobalPar.Punch.t + 1.0f;
            outl[i] *= punchamp;
            outr[i] *= punchamp;
            NoteGlobalPar.Punch.t -= NoteGlobalPar.Punch.dt;
            if(NoteGlobalPar.Punch.t < 0.0f) {
                NoteGlobalPar.Punch.Enabled = 0;
                break;
            }
        }

    watch_punch(outl,synth.buffersize);

    if(ABOVE_AMPLITUDE_THRESHOLD(globaloldamplitude, globalnewamplitude))
        // Amplitude Interpolation
        for(int i = 0; i < synth.buffersize; ++i) {
            float tmpvol = INTERPOLATE_AMPLITUDE(globaloldamplitude,
                                                 globalnewamplitude,
                                                 i,
                                                 synth.buffersize);
            outl[i] *= tmpvol * NoteGlobalPar.Panning;
            outr[i] *= tmpvol * (1.0f - NoteGlobalPar.Panning);
        }
    else
        for(int i = 0; i < synth.buffersize; ++i) {
            outl[i] *= globalnewamplitude * NoteGlobalPar.Panning;
            outr[i] *= globalnewamplitude * (1.0f - NoteGlobalPar.Panning);
        }

    watch_amp_int(outl,synth.buffersize);

    // Apply legato-specific sound signal modifications
    legato.apply(*this, outl, outr);

    watch_legato(outl,synth.buffersize);

    // Check if the global amplitude is finished.
    // If it does, disable the note
    if(NoteGlobalPar.AmpEnvelope->finished()) {
        for(int i = 0; i < synth.buffersize; ++i) { //fade-out
            float tmp = 1.0f - (float)i / synth.buffersize_f;
            outl[i] *= tmp;
            outr[i] *= tmp;
        }
        finished_ = 1;
    }

    return 1;
}

bool PADnote::finished() const
{
    return finished_;
}

void PADnote::entomb(void)
{
    NoteGlobalPar.AmpEnvelope->forceFinish();
}

void PADnote::releasekey()
{
    NoteGlobalPar.FreqEnvelope->releasekey();
    NoteGlobalPar.FilterEnvelope->releasekey();
    NoteGlobalPar.AmpEnvelope->releasekey();
}

}
