/*
  ZynAddSubFX - a software synthesizer

  pADnote.cpp - The "pad" synthesizer
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/
#include <math.h>
#include "PADnote.h"
#include "../Misc/Config.h"
#include "../Misc/Allocator.h"
#include "../DSP/Filter.h"

PADnote::PADnote(PADnoteParameters *parameters,
                 SynthParams pars)
    :SynthNote(pars), pars(*parameters)
{
    firsttick = true;
    setup(pars.frequency, pars.velocity, pars.portamento, pars.note);
}


void PADnote::setup(float freq,
                    float velocity,
                    bool  portamento_,
                    int   midinote,
                    bool  legato)
{
    portamento     = portamento_;
    this->velocity = velocity;
    finished_      = false;


    basefreq = applyFixedFreqET(pars.Pfixedfreq, freq, pars.PfixedfreqET,
            0.0, midinote);

    firsttick = true;
    realfreq  = basefreq;
    if(!legato)
        Detune = getdetune(pars.PDetuneType, pars.PCoarseDetune, pars.PDetune);


    //find out the closest note
    const float logfreq = logf(basefreq * powf(2.0f, Detune / 1200.0f));
    float mindist = fabs(logfreq - logf(pars.sample[0].basefreq + 0.0001f));
    nsample = 0;
    for(int i = 1; i < PAD_MAX_SAMPLES; ++i) {
        if(pars.sample[i].smp == NULL)
            break;
        float dist = fabs(logfreq - logf(pars.sample[i].basefreq + 0.0001f));

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


    if(pars.PPanning == 0)
        panning = RND;
    else
        panning = pars.PPanning / 128.0f;

    FilterCenterPitch = pars.GlobalFilter->getfreq() //center freq
                                      + pars.PFilterVelocityScale / 127.0f
                                      * 6.0f                                //velocity sensing
                                      * (VelF(velocity,
                                              pars.PFilterVelocityScaleFunction) - 1);

    if(!legato) {
        NoteGlobalPar.Punch.init(pars.PPunchStrength,
                pars.PPunchVelocitySensing, pars.PPunchTime,
                pars.PPunchStretch, velocity, freq);

        FreqEnvelope = memory.alloc<Envelope>(pars.FreqEnvelope, basefreq);
        NoteGlobalPar.FreqLfo      = memory.alloc<LFO>(pars.FreqLfo, basefreq);

        AmpEnvelope = memory.alloc<Envelope>(pars.AmpEnvelope, basefreq);
        NoteGlobalPar.AmpLfo      = memory.alloc<LFO>(pars.AmpLfo, basefreq);
    }

    volume = 4.0f * powf(0.1f, 3.0f * (1.0f - pars.PVolume / 96.0f))      //-60 dB .. 0 dB
        * VelF(velocity, pars.PAmpVelocityScaleFunction); //velocity sensing

    AmpEnvelope->envout_dB(); //discard the first envelope output
    oldamplitude = newamplitude = volume
                                              * AmpEnvelope->envout_dB()
                                              * NoteGlobalPar.AmpLfo->amplfoout();

    if(!legato) {
        GlobalFilterL = Filter::generate(memory, pars.GlobalFilter);
        GlobalFilterR = Filter::generate(memory, pars.GlobalFilter);

        FilterEnvelope = memory.alloc<Envelope>(pars.FilterEnvelope, basefreq);
        NoteGlobalPar.FilterLfo      = memory.alloc<LFO>(pars.FilterLfo, basefreq);
    }
    FilterQ = pars.GlobalFilter->getq();
    FilterFreqTracking = pars.GlobalFilter->getfreqtracking(basefreq);

    if(!pars.sample[nsample].smp) {
        finished_ = true;
        return;
    }
}

void PADnote::legatonote(LegatoParams pars)
{
    // Manage legato stuff
    if(legato.update(pars))
        return;

    setup(pars.frequency, pars.velocity, pars.portamento, pars.midinote, true);
}


PADnote::~PADnote()
{
    memory.dealloc(FreqEnvelope);
    memory.dealloc(NoteGlobalPar.FreqLfo);
    memory.dealloc(AmpEnvelope);
    memory.dealloc(NoteGlobalPar.AmpLfo);
    memory.dealloc(GlobalFilterL);
    memory.dealloc(GlobalFilterR);
    memory.dealloc(FilterEnvelope);
    memory.dealloc(NoteGlobalPar.FilterLfo);
}

void PADnote::computecurrentparameters()
{
    const float globalpitch = 0.01f * (FreqEnvelope->envout()
                           + NoteGlobalPar.FreqLfo->lfoout()
                           * ctl.modwheel.relmod + Detune);
    oldamplitude = newamplitude;
    newamplitude = volume * AmpEnvelope->envout_dB()
                         * NoteGlobalPar.AmpLfo->amplfoout();

    const float globalfilterpitch = FilterEnvelope->envout()
                        + NoteGlobalPar.FilterLfo->lfoout()
                        + FilterCenterPitch;

    float tmpfilterfreq = globalfilterpitch + ctl.filtercutoff.relfreq
                          + FilterFreqTracking;

    tmpfilterfreq = Filter::getrealfreq(tmpfilterfreq);

    const float globalfilterq = FilterQ * ctl.filterq.relq;
    GlobalFilterL->setfreq_and_q(tmpfilterfreq, globalfilterq);
    GlobalFilterR->setfreq_and_q(tmpfilterfreq, globalfilterq);

    //compute the portamento, if it is used by this note
    const float portamentofreqrap = getPortamento(portamento);

    realfreq = basefreq * portamentofreqrap
               * powf(2.0f, globalpitch / 12.0f) * ctl.pitchwheel.relfreq;
}


int PADnote::Compute_Linear(float *outl,
                            float *outr,
                            int freqhi,
                            float freqlo)
{
    float *smps = pars.sample[nsample].smp;
    if(smps == NULL) {
        finished_ = true;
        return 1;
    }
    int size = pars.sample[nsample].size;
    for(int i = 0; i < synth->buffersize; ++i) {
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
    for(int i = 0; i < synth->buffersize; ++i) {
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
    const float *smps = pars.sample[nsample].smp;
    if(smps == NULL) {
        for(int i = 0; i < synth->buffersize; ++i) {
            outl[i] = 0.0f;
            outr[i] = 0.0f;
        }
        return 1;
    }
    const float smpfreq = pars.sample[nsample].basefreq;


    const float freqrap = realfreq / smpfreq;
    const int   freqhi  = (int) (floor(freqrap));
    const float freqlo  = freqrap - floor(freqrap);


    if(config.cfg.Interpolation)
        Compute_Cubic(outl, outr, freqhi, freqlo);
    else
        Compute_Linear(outl, outr, freqhi, freqlo);


    if(firsttick) {
        fadein(outl);
        fadein(outr);
        firsttick = false;
    }

    GlobalFilterL->filterout(outl);
    GlobalFilterR->filterout(outr);

    //Apply the punch
    applyPunch(outl, outr, NoteGlobalPar.Punch);

    applyPanning(outl, outr, oldamplitude, newamplitude,
            panning, (1.0f - panning));
                 

    // Apply legato-specific sound signal modifications
    legato.apply(*this, outl, outr);

    // Check if the global amplitude is finished.
    // If it does, disable the note
    if(AmpEnvelope->finished()) {
        fadeout(outl);
        fadeout(outr);
        finished_ = true;
    }

    return 1;
}

bool PADnote::finished() const
{
    return finished_;
}

void PADnote::relasekey()
{
    FreqEnvelope->relasekey();
    FilterEnvelope->relasekey();
    AmpEnvelope->relasekey();
}
