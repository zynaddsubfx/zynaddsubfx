/*
  ZynAddSubFX - a software synthesizer

  ADnote.cpp - The "additive" synthesizer
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

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <stdint.h>

#include "../globals.h"
#include "../Misc/Util.h"
#include "../Misc/Allocator.h"
#include "../Params/Controller.h"
#include "../DSP/Filter.h"
#include "OscilGen.h"
#include "ADnote.h"


ADnote::ADnote(ADnoteParameters *pars, SynthParams &spars)
    :SynthNote(spars), oscSize(synth->oscilsize)
{
    tmpwavel = memory.valloc<float>(synth->buffersize);
    tmpwaver = memory.valloc<float>(synth->buffersize);
    bypassl  = memory.valloc<float>(synth->buffersize);
    bypassr  = memory.valloc<float>(synth->buffersize);

    partparams = pars;
    portamento  = spars.portamento;
    midinote    = spars.note;
    NoteEnabled = true;
    basefreq    = spars.frequency;
    velocity    = spars.velocity;
    time   = 0.0f;
    stereo = pars->GlobalPar.PStereo;

    Detune = getdetune(pars->GlobalPar.PDetuneType,
                                     pars->GlobalPar.PCoarseDetune,
                                     pars->GlobalPar.PDetune);
    bandwidthDetuneMultiplier = pars->getBandwidthDetuneMultiplier();

    if(pars->GlobalPar.PPanning == 0)
        panning = RND;
    else
        panning = pars->GlobalPar.PPanning / 128.0f;


    FilterCenterPitch = pars->GlobalPar.GlobalFilter->getfreq() //center freq
                                      + pars->GlobalPar.PFilterVelocityScale
                                      / 127.0f * 6.0f                                  //velocity sensing
                                      * (VelF(velocity,
                                              pars->GlobalPar.
                                              PFilterVelocityScaleFunction) - 1);

    if(pars->GlobalPar.PPunchStrength) {
        NoteGlobalPar.Punch.Enabled = true;
        NoteGlobalPar.Punch.t = 1.0f; //start from 1.0f and to 0.0f
        NoteGlobalPar.Punch.initialvalue =
            ((powf(10, 1.5f * pars->GlobalPar.PPunchStrength / 127.0f) - 1.0f)
             * VelF(velocity,
                    pars->GlobalPar.PPunchVelocitySensing));
        float time =
            powf(10, 3.0f * pars->GlobalPar.PPunchTime / 127.0f) / 10000.0f;   //0.1f .. 100 ms
        float stretch = powf(440.0f / spars.frequency,
                             pars->GlobalPar.PPunchStretch / 64.0f);
        NoteGlobalPar.Punch.dt = 1.0f / (time * synth->samplerate_f * stretch);
    }
    else
        NoteGlobalPar.Punch.Enabled = false;

    for(int nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
        auto &nvp = NoteVoicePar[nvoice];
        auto &pvp = pars->VoicePar[nvoice];
        auto &nvi = NoteVoiceInternals[nvoice];
        pars->VoicePar[nvoice].OscilSmp->newrandseed(prng());
        nvp.OscilSmp = NULL;
        nvp.FMSmp    = NULL;
        nvp.VoiceOut = NULL;

        nvp.FMVoice = -1;
        nvi.unison_size = 1;

        if(!pvp.Enabled) {
            nvp.Enabled = false;
            continue; //the voice is disabled
        }

        nvi.unison_stereo_spread = pvp.Unison_stereo_spread / 127.0f;
        const int unison = pvp.Unison_size < 1 ? 1 : pvp.Unison_size;

        //compute unison
        nvi.unison_size = unison;

        nvi.unison_base_freq_rap = memory.valloc<float>(unison);
        nvi.unison_freq_rap      = memory.valloc<float>(unison);
        nvi.unison_invert_phase  = memory.valloc<bool>(unison);
        float unison_spread = pars->getUnisonFrequencySpreadCents(nvoice);
        float unison_real_spread = powf(2.0f, (unison_spread * 0.5f) / 1200.0f);
        float unison_vibratto_a  = pvp.Unison_vibratto / 127.0f; //0.0f .. 1.0f


        switch(unison) {
            case 1:
                nvi.unison_base_freq_rap[0] = 1.0f; //if the unison is not used, always make the only subvoice to have the default note
                break;
            case 2: { //unison for 2 subvoices
                nvi.unison_base_freq_rap[0] = 1.0f / unison_real_spread;
                nvi.unison_base_freq_rap[1] = unison_real_spread;
            };
                break;
            default: { //unison for more than 2 subvoices
                float unison_values[unison];
                float min = -1e-6, max = 1e-6;
                for(int k = 0; k < unison; ++k) {
                    const float step = (k / (float) (unison - 1)) * 2.0f - 1.0f; //this makes the unison spread more uniform
                    const float val  = step + (RND * 2.0f - 1.0f) / (unison - 1);
                    unison_values[k] = val;
                    if(min > val)
                        min = val;
                    if(max < val)
                        max = val;
                }
                const float diff = max - min;
                for(int k = 0; k < unison; ++k) {
                    unison_values[k] =
                        (unison_values[k] - (max + min) * 0.5f) / diff;             //the lowest value will be -1 and the highest will be 1
                    nvi.unison_base_freq_rap[k] =
                        powf(2.0f, (unison_spread * unison_values[k]) / 1200);
                }
            };
        }

        //unison vibrattos
        if(unison > 1)
            for(int k = 0; k < unison; ++k) //reduce the frequency difference for larger vibrattos
                nvi.unison_base_freq_rap[k] = 1.0f + (nvi.unison_base_freq_rap[k] - 1.0f)
                                                  * (1.0f - unison_vibratto_a);
        nvi.unison_vibratto.step      = memory.valloc<float>(unison);
        nvi.unison_vibratto.position  = memory.valloc<float>(unison);
        nvi.unison_vibratto.amplitude =
            (unison_real_spread - 1.0f) * unison_vibratto_a;

        float increments_per_second = synth->samplerate_f / synth->buffersize_f;
        float vibratto_base_period  = 0.25f * powf(2.0f,
            (1.0f - pvp.Unison_vibratto_speed / 127.0f) * 4.0f);
        for(int k = 0; k < unison; ++k) {
            nvi.unison_vibratto.position[k] = RND * 1.8f - 0.9f;
            //make period to vary randomly from 50% to 200% vibratto base period
            float vibratto_period = vibratto_base_period
                                    * powf(2.0f, RND * 2.0f - 1.0f);

            float m = 4.0f / (vibratto_period * increments_per_second);
            if(RND < 0.5f)
                m = -m;
            nvi.unison_vibratto.step[k] = m;
        }

        if(unison == 1) { //no vibratto for a single voice
            nvi.unison_vibratto.step[0]     = 0.0f;
            nvi.unison_vibratto.position[0] = 0.0f;
            nvi.unison_vibratto.amplitude   = 0.0f;
        }

        //phase invert for unison
        nvi.unison_invert_phase[0] = false;
        if(unison != 1) {
            int inv = pvp.Unison_invert_phase;
            switch(inv) {
                case 0: for(int k = 0; k < unison; ++k)
                        nvi.unison_invert_phase[k] = false;
                    break;
                case 1: for(int k = 0; k < unison; ++k)
                        nvi.unison_invert_phase[k] = (RND > 0.5f);
                    break;
                default: for(int k = 0; k < unison; ++k)
                        nvi.unison_invert_phase[k] =
                            (k % inv == 0) ? true : false;
                    break;
            }
        }


        nvi.oscfreqhi   = memory.valloc<int>(unison);
        nvi.oscfreqlo   = memory.valloc<float>(unison);
        nvi.oscfreqhiFM = memory.valloc<unsigned int>(unison);
        nvi.oscfreqloFM = memory.valloc<float>(unison);
        nvi.oscposhi    = memory.valloc<int>(unison);
        nvi.oscposlo    = memory.valloc<float>(unison);
        nvi.oscposhiFM  = memory.valloc<unsigned int>(unison);
        nvi.oscposloFM  = memory.valloc<float>(unison);

        nvp.Enabled     = true;
        nvp.fixedfreq   = pvp.Pfixedfreq;
        nvp.fixedfreqET = pvp.PfixedfreqET;

        //use the Globalpars.detunetype if the detunetype is 0
        if(pvp.PDetuneType != 0) {
            //Coarse detune
            nvp.Detune = getdetune(pvp.PDetuneType, pvp.PCoarseDetune, 8192);
            //Fine detune
            nvp.FineDetune = getdetune(pvp.PDetuneType, 0, pvp.PDetune);
        }
        else {
            nvp.Detune = getdetune(
                pars->GlobalPar.PDetuneType,
                pvp.PCoarseDetune,
                8192); //coarse detune
            nvp.FineDetune = getdetune(
                pars->GlobalPar.PDetuneType,
                0,
                pvp.PDetune); //fine detune
        }
        if(pvp.PFMDetuneType != 0)
            nvp.FMDetune = getdetune(pvp.PFMDetuneType,
                    pvp.PFMCoarseDetune, pvp.PFMDetune);
        else
            nvp.FMDetune = getdetune(pars->GlobalPar.PDetuneType,
                    pvp.PFMCoarseDetune, pvp.PFMDetune);



        for(int k = 0; k < unison; ++k) {
            nvi.oscposhi[k]   = 0;
            nvi.oscposlo[k]   = 0.0f;
            nvi.oscposhiFM[k] = 0;
            nvi.oscposloFM[k] = 0.0f;
        }

        //the extra points contains the first point
        nvp.OscilSmp = memory.valloc<float>(oscSize + OSCIL_SMP_EXTRA_SAMPLES);

        //Get the voice's oscil or external's voice oscil
        int vc = nvoice;
        if(pvp.Pextoscil != -1)
            vc = pvp.Pextoscil;
        if(!pars->GlobalPar.Hrandgrouping)
            pars->VoicePar[vc].OscilSmp->newrandseed(prng());
        int oscposhi_start =
            pars->getVoiceOsc(vc, nvp.OscilSmp,
                    getvoicebasefreq(nvoice),
                    pvp.Presonance);

        //I store the first elments to the last position for speedups
        for(int i = 0; i < OSCIL_SMP_EXTRA_SAMPLES; ++i)
            nvp.OscilSmp[oscSize + i] = nvp.OscilSmp[i];

        oscposhi_start +=
            (int)((pvp.Poscilphase - 64.0f) / 128.0f * oscSize + oscSize * 4);
        oscposhi_start %= oscSize;

        for(int k = 0; k < unison; ++k) {
            nvi.oscposhi[k] = oscposhi_start;
            //put random starting point for other subvoices
            oscposhi_start      =
                RND * pvp.Unison_phase_randomness / 127.0f * (oscSize - 1);
        }

        nvp.FreqLfo      = NULL;
        nvp.FreqEnvelope = NULL;

        nvp.AmpLfo      = NULL;
        nvp.AmpEnvelope = NULL;

        nvp.VoiceFilterL   = NULL;
        nvp.VoiceFilterR   = NULL;
        nvp.FilterEnvelope = NULL;
        nvp.FilterLfo      = NULL;

        nvp.FilterCenterPitch = pvp.VoiceFilter->getfreq();
        nvp.filterbypass = pvp.Pfilterbypass;

        switch(pvp.PFMEnabled) {
            case 1:
                nvp.FMEnabled = MORPH;
                break;
            case 2:
                nvp.FMEnabled = RING_MOD;
                break;
            case 3:
                nvp.FMEnabled = PHASE_MOD;
                break;
            case 4:
                nvp.FMEnabled = FREQ_MOD;
                break;
            case 5:
                nvp.FMEnabled = PITCH_MOD;
                break;
            default:
                nvp.FMEnabled = NONE;
        }

        nvp.FMVoice = pvp.PFMVoice;
        nvp.FMFreqEnvelope = NULL;
        nvp.FMAmpEnvelope  = NULL;

        //Compute the Voice's modulator volume (incl. damping)
        float fmvoldamp = powf(440.0f / getvoicebasefreq(nvoice),
                               pvp.PFMVolumeDamp / 64.0f - 1.0f);
        switch(nvp.FMEnabled) {
            case PHASE_MOD:
                fmvoldamp =
                    powf(440.0f / getvoicebasefreq(nvoice), pvp.PFMVolumeDamp
                         / 64.0f);
                nvp.FMVolume =
                    (expf(pvp.PFMVolume / 127.0f
                          * FM_AMP_MULTIPLIER) - 1.0f) * fmvoldamp * 4.0f;
                break;
            case FREQ_MOD:
                nvp.FMVolume =
                    (expf(pvp.PFMVolume / 127.0f
                          * FM_AMP_MULTIPLIER) - 1.0f) * fmvoldamp * 4.0f;
                break;
            default:
                if(fmvoldamp > 1.0f)
                    fmvoldamp = 1.0f;
                nvp.FMVolume = pvp.PFMVolume / 127.0f * fmvoldamp;
        }

        //Voice's modulator velocity sensing
        nvp.FMVolume *=
            VelF(velocity, pvp.PFMVelocityScaleFunction);

        nvi.FMoldsmp = memory.valloc<float>(unison);
        for(int k = 0; k < unison; ++k)
            nvi.FMoldsmp[k] = 0.0f; //this is for FM (integration)

        nvi.firsttick = true;
        nvp.DelayTicks =
            (int)((expf(pvp.PDelay / 127.0f
                        * logf(50.0f))
                   - 1.0f) / synth->buffersize_f / 10.0f * synth->samplerate_f);
    }

    max_unison = 1;
    for(int nvoice = 0; nvoice < NUM_VOICES; ++nvoice)
        if(NoteVoiceInternals[nvoice].unison_size > max_unison)
            max_unison = NoteVoiceInternals[nvoice].unison_size;


    tmpwave_unison = memory.valloc<float*>(max_unison);
    for(int k = 0; k < max_unison; ++k) {
        tmpwave_unison[k] = memory.valloc<float>(synth->buffersize);
        memset(tmpwave_unison[k], 0, synth->bufferbytes);
    }

    initparameters();
}

// ADlegatonote: This function is (mostly) a copy of ADnote(...) and
// initparameters() stuck together with some lines removed so that it
// only alter the already playing note (to perform legato). It is
// possible I left stuff that is not required for this.
void ADnote::legatonote(LegatoParams lpars)
{
    ADnoteParameters *pars = partparams;

    // Manage legato stuff
    if(legato.update(lpars))
        return;

    portamento = lpars.portamento;
    midinote   = lpars.midinote;
    basefreq   = lpars.frequency;

    if(velocity > 1.0f)
        velocity = 1.0f;
    velocity = lpars.velocity;

    Detune = getdetune(pars->GlobalPar.PDetuneType,
                                     pars->GlobalPar.PCoarseDetune,
                                     pars->GlobalPar.PDetune);
    bandwidthDetuneMultiplier = pars->getBandwidthDetuneMultiplier();

    if(pars->GlobalPar.PPanning == 0)
        panning = RND;
    else
        panning = pars->GlobalPar.PPanning / 128.0f;

    //center freq
    FilterCenterPitch =
        pars->GlobalPar.GlobalFilter->getfreq()
        + pars->GlobalPar.PFilterVelocityScale / 127.0f * 6.0f//velocity sensing
        * (VelF(velocity, pars->GlobalPar.  PFilterVelocityScaleFunction) - 1);


    for(int nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
        auto &nvp = NoteVoicePar[nvoice];
        auto &pvp = pars->VoicePar[nvoice];
        if(!nvp.Enabled)
            continue;  //(gf) Stay the same as first note in legato.

        nvp.fixedfreq   = pvp.Pfixedfreq;
        nvp.fixedfreqET = pvp.PfixedfreqET;

        //use the Globalpars.detunetype if the detunetype is 0
        if(pvp.PDetuneType != 0) {
            nvp.Detune = getdetune(
                pvp.PDetuneType,
                pvp.PCoarseDetune,
                8192); //coarse detune
            nvp.FineDetune = getdetune(
                pvp.PDetuneType,
                0,
                pvp.PDetune); //fine detune
        }
        else {
            nvp.Detune = getdetune(
                pars->GlobalPar.PDetuneType,
                pvp.PCoarseDetune,
                8192); //coarse detune
            nvp.FineDetune = getdetune(
                pars->GlobalPar.PDetuneType,
                0,
                pvp.PDetune); //fine detune
        }
        if(pvp.PFMDetuneType != 0)
            nvp.FMDetune = getdetune(
                pvp.PFMDetuneType,
                pvp.PFMCoarseDetune,
                pvp.PFMDetune);
        else
            nvp.FMDetune = getdetune(
                pars->GlobalPar.PDetuneType,
                pvp.PFMCoarseDetune,
                pvp.PFMDetune);


        //Get the voice's oscil or external's voice oscil
        const bool resonance = pvp.Presonance;
        int vc = nvoice;
        if(pvp.Pextoscil != -1)
            vc = pvp.Pextoscil;
        if(!pars->GlobalPar.Hrandgrouping)
            pars->VoicePar[vc].OscilSmp->newrandseed(prng());

        pars->getVoiceOsc(vc, nvp.OscilSmp,
                getvoicebasefreq(nvoice), resonance);

        //I store the first elments to the last position for speedups
        for(int i = 0; i < OSCIL_SMP_EXTRA_SAMPLES; ++i)
            nvp.OscilSmp[oscSize + i] =
                nvp.OscilSmp[i];


        nvp.FilterCenterPitch =
            pvp.VoiceFilter->getfreq();
        nvp.filterbypass =
            pvp.Pfilterbypass;


        nvp.FMVoice = pvp.PFMVoice;

        //Compute the Voice's modulator volume (incl. damping)
        float fmvoldamp = powf(440.0f / getvoicebasefreq(nvoice),
                               pvp.PFMVolumeDamp / 64.0f
                               - 1.0f);

        switch(nvp.FMEnabled) {
            case PHASE_MOD:
                fmvoldamp =
                    powf(440.0f / getvoicebasefreq(
                             nvoice), pvp.PFMVolumeDamp
                         / 64.0f);
                nvp.FMVolume =
                    (expf(pvp.PFMVolume / 127.0f
                          * FM_AMP_MULTIPLIER) - 1.0f) * fmvoldamp * 4.0f;
                break;
            case FREQ_MOD:
                nvp.FMVolume =
                    (expf(pvp.PFMVolume / 127.0f
                          * FM_AMP_MULTIPLIER) - 1.0f) * fmvoldamp * 4.0f;
                break;
            default:
                if(fmvoldamp > 1.0f)
                    fmvoldamp = 1.0f;
                nvp.FMVolume =
                    pvp.PFMVolume
                    / 127.0f * fmvoldamp;
        }

        //Voice's modulator velocity sensing
        nvp.FMVolume *=
            VelF(velocity,
                 pvp.PFMVelocityScaleFunction);

        nvp.DelayTicks =
            (int)((expf(pvp.PDelay / 127.0f * logf(50.0f))
                   - 1.0f) / synth->buffersize_f / 10.0f * synth->samplerate_f);
    }

    ///    initparameters();

    ///////////////
    // Altered content of initparameters():

    int tmp[NUM_VOICES];

    volume = 4.0f
        * powf(0.1f, 3.0f
                * (1.0f - partparams->GlobalPar.PVolume
                    / 96.0f))                                      //-60 dB .. 0 dB
        * VelF(velocity, partparams->GlobalPar.PAmpVelocityScaleFunction);
    //velocity sensing

    newamplitude = volume * AmpEnvelope->envout_dB()
        * NoteGlobalPar.AmpLfo->amplfoout();

    FilterQ = partparams->GlobalPar.GlobalFilter->getq();
    FilterFreqTracking =
        partparams->GlobalPar.GlobalFilter->getfreqtracking(basefreq);

    // Forbids the Modulation Voice to be greater or equal than voice
    for(int i = 0; i < NUM_VOICES; ++i)
        if(NoteVoicePar[i].FMVoice >= i)
            NoteVoicePar[i].FMVoice = -1;

    // Voice Parameter init
    for(unsigned nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
        auto &nvp = NoteVoicePar[nvoice];
        auto &pvp = partparams->VoicePar[nvoice];
        auto &nvi = NoteVoiceInternals[nvoice];
        if(!nvp.Enabled)
            continue;

        nvp.noisetype = pvp.Type;
        /* Voice Amplitude Parameters Init */
                          // -60 dB .. 0 dB
        nvp.Volume = powf(0.1f, 3.0f * (1.0f - pvp.PVolume / 127.0f))
            * VelF(velocity, pvp.PAmpVelocityScaleFunction); //velocity

        if(pvp.PVolumeminus)
            nvp.Volume = -nvp.Volume;

        if(pvp.PPanning == 0)
            nvp.Panning = RND;  // random panning
        else
            nvp.Panning = pvp.PPanning / 128.0f;

        nvi.newamplitude = 1.0f;
        if(pvp.PAmpEnvelopeEnabled && nvp.AmpEnvelope)
            nvi.newamplitude *= nvp.AmpEnvelope->envout_dB();

        if(pvp.PAmpLfoEnabled && nvp.AmpLfo)
            nvi.newamplitude *= nvp.AmpLfo->amplfoout();

        nvp.FilterFreqTracking =
            pvp.VoiceFilter->getfreqtracking(basefreq);

        /* Voice Modulation Parameters Init */
        if((nvp.FMEnabled != NONE)
           && (nvp.FMVoice < 0)) {
            pvp.FMSmp->newrandseed(prng());

            //Perform Anti-aliasing only on MORPH or RING MODULATION

            int vc = nvoice;
            if(pvp.PextFMoscil != -1)
                vc = pvp.PextFMoscil;

            if(!partparams->GlobalPar.Hrandgrouping)
                partparams->VoicePar[vc].FMSmp->newrandseed(prng());

            for(int i = 0; i < OSCIL_SMP_EXTRA_SAMPLES; ++i)
                nvp.FMSmp[oscSize + i] = nvp.FMSmp[i];
        }

        nvi.FMnewamplitude = nvp.FMVolume * ctl.fmamp.relamp;

        if(pvp.PFMAmpEnvelopeEnabled && nvp.FMAmpEnvelope)
            nvi.FMnewamplitude *= nvp.FMAmpEnvelope->envout_dB();
    }

    for(int nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
        for(unsigned i = nvoice + 1; i < NUM_VOICES; ++i)
            tmp[i] = 0;
        for(unsigned i = nvoice + 1; i < NUM_VOICES; ++i)
            if((NoteVoicePar[i].FMVoice == nvoice) && (tmp[i] == 0))
                tmp[i] = 1;
    }
}


/*
 * Kill a voice of ADnote
 */
void ADnote::KillVoice(int nvoice)
{
    auto &nvi = NoteVoiceInternals[nvoice];
    memory.devalloc(nvi.oscfreqhi);
    memory.devalloc(nvi.oscfreqlo);
    memory.devalloc(nvi.oscfreqhiFM);
    memory.devalloc(nvi.oscfreqloFM);
    memory.devalloc(nvi.oscposhi);
    memory.devalloc(nvi.oscposlo);
    memory.devalloc(nvi.oscposhiFM);
    memory.devalloc(nvi.oscposloFM);

    memory.devalloc(nvi.unison_base_freq_rap);
    memory.devalloc(nvi.unison_freq_rap);
    memory.devalloc(nvi.unison_invert_phase);
    memory.devalloc(nvi.FMoldsmp);
    memory.devalloc(nvi.unison_vibratto.step);
    memory.devalloc(nvi.unison_vibratto.position);
    
    memory.dealloc(FreqEnvelope);
    memory.dealloc(AmpEnvelope);
    memory.dealloc(GlobalFilterL);
    memory.dealloc(GlobalFilterR);
    memory.dealloc(FilterEnvelope);

    NoteVoicePar[nvoice].kill(memory);
}

/*
 * Kill the note
 */
void ADnote::KillNote()
{
    for(unsigned nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
        if(NoteVoicePar[nvoice].Enabled)
            KillVoice(nvoice);

        if(NoteVoicePar[nvoice].VoiceOut)
            memory.dealloc(NoteVoicePar[nvoice].VoiceOut);
    }

    NoteGlobalPar.kill(memory);

    NoteEnabled = false;
}

ADnote::~ADnote()
{
    if(NoteEnabled)
        KillNote();
    memory.devalloc(tmpwavel);
    memory.devalloc(tmpwaver);
    memory.devalloc(bypassl);
    memory.devalloc(bypassr);
    for(int k = 0; k < max_unison; ++k)
        memory.devalloc(tmpwave_unison[k]);
    memory.devalloc(tmpwave_unison);
}


/*
 * Init the parameters
 */
void ADnote::initparameters()
{
    int tmp[NUM_VOICES];

    // Global Parameters
    auto &gbp = partparams->GlobalPar;
    FreqEnvelope = memory.alloc<Envelope>(gbp.FreqEnvelope, basefreq);

    AmpEnvelope = memory.alloc<Envelope>(gbp.AmpEnvelope, basefreq);

    volume = 4.0f * powf(0.1f, 3.0f * (1.0f - gbp.PVolume / 96.0f)) //-60 dB .. 0 dB
             * VelF(velocity, gbp.PAmpVelocityScaleFunction);     //sensing

    GlobalFilterL = Filter::generate(memory, gbp.GlobalFilter);
    if(stereo)
        GlobalFilterR = Filter::generate(memory, gbp.GlobalFilter);
    else
        GlobalFilterR = NULL;

    FilterEnvelope = memory.alloc<Envelope>(gbp.FilterEnvelope, basefreq);
    FilterQ = gbp.GlobalFilter->getq();
    FilterFreqTracking = gbp.GlobalFilter->getfreqtracking(basefreq);
    
    NoteGlobalPar.initparameters(partparams->GlobalPar,
                                 memory, basefreq, velocity,
                                 stereo);

    AmpEnvelope->envout_dB(); //discard the first envelope output
    newamplitude = volume * AmpEnvelope->envout_dB()
        * NoteGlobalPar.AmpLfo->amplfoout();

    // Forbids the Modulation Voice to be greater or equal than voice
    for(int i = 0; i < NUM_VOICES; ++i)
        if(NoteVoicePar[i].FMVoice >= i)
            NoteVoicePar[i].FMVoice = -1;

    // Voice Parameter init
    for(int nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
        Voice &vce = NoteVoicePar[nvoice];
        auto  &nvi = NoteVoiceInternals[nvoice];
        ADnoteVoiceParam &param = partparams->VoicePar[nvoice];

        if(!vce.Enabled)
            continue;

        vce.noisetype = param.Type;
        /* Voice Amplitude Parameters Init */
        vce.Volume = powf(0.1f, 3.0f * (1.0f - param.PVolume / 127.0f)) // -60dB..0dB
                     * VelF(velocity, param.PAmpVelocityScaleFunction);

        if(param.PVolumeminus)
            vce.Volume = -vce.Volume;

        if(param.PPanning == 0)
            vce.Panning = RND;  // random panning
        else
            vce.Panning = param.PPanning / 128.0f;

        nvi.newamplitude = 1.0f;
        if(param.PAmpEnvelopeEnabled) {
            vce.AmpEnvelope = memory.alloc<Envelope>(param.AmpEnvelope, basefreq);
            vce.AmpEnvelope->envout_dB(); //discard the first envelope sample
            nvi.newamplitude *= vce.AmpEnvelope->envout_dB();
        }

        if(param.PAmpLfoEnabled) {
            vce.AmpLfo = memory.alloc<LFO>(param.AmpLfo, basefreq);
            nvi.newamplitude *= vce.AmpLfo->amplfoout();
        }

        /* Voice Frequency Parameters Init */
        if(param.PFreqEnvelopeEnabled)
            vce.FreqEnvelope = memory.alloc<Envelope>(param.FreqEnvelope, basefreq);

        if(param.PFreqLfoEnabled)
            vce.FreqLfo = memory.alloc<LFO>(param.FreqLfo, basefreq);

        /* Voice Filter Parameters Init */
        if(param.PFilterEnabled) {
            vce.VoiceFilterL = Filter::generate(memory, param.VoiceFilter);
            vce.VoiceFilterR = Filter::generate(memory, param.VoiceFilter);
        }

        if(param.PFilterEnvelopeEnabled)
            vce.FilterEnvelope = memory.alloc<Envelope>(param.FilterEnvelope, basefreq);

        if(param.PFilterLfoEnabled)
            vce.FilterLfo = memory.alloc<LFO>(param.FilterLfo, basefreq);

        vce.FilterFreqTracking =
            param.VoiceFilter->getfreqtracking(basefreq);

        /* Voice Modulation Parameters Init */
        if((vce.FMEnabled != NONE) && (vce.FMVoice < 0)) {
            param.FMSmp->newrandseed(prng());
            vce.FMSmp = memory.valloc<float>(oscSize + OSCIL_SMP_EXTRA_SAMPLES);

            //Perform Anti-aliasing only on MORPH or RING MODULATION

            int vc = nvoice;
            if(param.PextFMoscil != -1)
                vc = param.PextFMoscil;

            float tmp = 1.0f;
            if((partparams->VoicePar[vc].FMSmp->Padaptiveharmonics)
               || (vce.FMEnabled == MORPH)
               || (vce.FMEnabled == RING_MOD))
                tmp = getFMvoicebasefreq(nvoice);

            if(!partparams->GlobalPar.Hrandgrouping)
                partparams->VoicePar[vc].FMSmp->newrandseed(prng());

            for(int k = 0; k < nvi.unison_size; ++k) {
                short ttmp = partparams->getVoiceMod(vc, vce.FMSmp, tmp);
                nvi.oscposhiFM[k] = (nvi.oscposhi[k] + ttmp) % oscSize;
            }

            for(int i = 0; i < OSCIL_SMP_EXTRA_SAMPLES; ++i)
                vce.FMSmp[oscSize + i] = vce.FMSmp[i];
            int oscposhiFM_add =
                (int)((param.PFMoscilphase
                       - 64.0f) / 128.0f * oscSize
                      + oscSize * 4);
            for(int k = 0; k < nvi.unison_size; ++k) {
                nvi.oscposhiFM[k] += oscposhiFM_add;
                nvi.oscposhiFM[k] %= oscSize;
            }
        }

        if(param.PFMFreqEnvelopeEnabled)
            vce.FMFreqEnvelope = memory.alloc<Envelope>(param.FMFreqEnvelope, basefreq);

        nvi.FMnewamplitude = vce.FMVolume * ctl.fmamp.relamp;

        if(param.PFMAmpEnvelopeEnabled) {
            vce.FMAmpEnvelope =
                memory.alloc<Envelope>(param.FMAmpEnvelope, basefreq);
            nvi.FMnewamplitude *= vce.FMAmpEnvelope->envout_dB();
        }
    }

    for(int nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
        auto &nvp = NoteVoicePar[nvoice];
        for(int i = nvoice + 1; i < NUM_VOICES; ++i)
            tmp[i] = 0;
        for(int i = nvoice + 1; i < NUM_VOICES; ++i)
            if(NoteVoicePar[i].FMVoice == nvoice && tmp[i] == 0) {
                nvp.VoiceOut = memory.valloc<float>(synth->buffersize);
                tmp[i] = 1;
            }

        if(nvp.VoiceOut)
            memset(nvp.VoiceOut, 0, synth->bufferbytes);
    }
}


/*
 * Computes the relative frequency of each unison voice and it's vibratto
 * This must be called before setfreq* functions
 */
void ADnote::compute_unison_freq_rap(int nvoice) {
    auto &nvi = NoteVoiceInternals[nvoice];
    if(nvi.unison_size == 1) { //no unison
        nvi.unison_freq_rap[0] = 1.0f;
        return;
    }
    float relbw = ctl.bandwidth.relbw * bandwidthDetuneMultiplier;
    for(int k = 0; k < nvi.unison_size; ++k) {
        float pos  = nvi.unison_vibratto.position[k];
        float step = nvi.unison_vibratto.step[k];
        pos += step;
        if(pos <= -1.0f) {
            pos  = -1.0f;
            step = -step;
        }
        if(pos >= 1.0f) {
            pos  = 1.0f;
            step = -step;
        }
        float vibratto_val = (pos - 0.333333333f * pos * pos * pos) * 1.5f; //make the vibratto lfo smoother
        nvi.unison_freq_rap[k] = 1.0f + ((nvi.unison_base_freq_rap[k]
                                         - 1.0f) + vibratto_val
                                        * nvi.unison_vibratto.amplitude)
                                     * relbw;

        nvi.unison_vibratto.position[k] = pos;
        step = nvi.unison_vibratto.step[k] = step;
    }
}


/*
 * Computes the frequency of an oscillator
 */
void ADnote::setfreq(int nvoice, float in_freq)
{
    auto &nvi = NoteVoiceInternals[nvoice];
    for(int k = 0; k < nvi.unison_size; ++k) {
        const float freq  = fabs(in_freq) * nvi.unison_freq_rap[k];
        float speed = freq * oscSize / synth->samplerate_f;
        if(speed > oscSize)
            speed = oscSize;

        F2I(speed, nvi.oscfreqhi[k]);
        nvi.oscfreqlo[k] = speed - floor(speed);
    }
}

/*
 * Computes the frequency of an modullator oscillator
 */
void ADnote::setfreqFM(int nvoice, float in_freq)
{
    auto &nvi = NoteVoiceInternals[nvoice];
    for(int k = 0; k < nvi.unison_size; ++k) {
        float freq  = fabs(in_freq) * nvi.unison_freq_rap[k];
        float speed = freq * oscSize / synth->samplerate_f;
        if(speed > synth->samplerate_f)
            speed = synth->samplerate_f;

        F2I(speed, nvi.oscfreqhiFM[k]);
        nvi.oscfreqloFM[k] = speed - floor(speed);
    }
}

/*
 * Get Voice base frequency
 */
float ADnote::getvoicebasefreq(int nvoice) const
{
    auto &nvp = NoteVoicePar[nvoice];
    float detune = nvp.Detune / 100.0f
                   + nvp.FineDetune / 100.0f
                   * ctl.bandwidth.relbw * bandwidthDetuneMultiplier
                   + Detune / 100.0f;

    return applyFixedFreqET(nvp.fixedfreq, basefreq, nvp.fixedfreqET,
            detune, midinote);
}

/*
 * Get Voice's Modullator base frequency
 */
float ADnote::getFMvoicebasefreq(int nvoice) const
{
    const float detune = NoteVoicePar[nvoice].FMDetune / 100.0f;
    return getvoicebasefreq(nvoice) * powf(2, detune / 12.0f);
}

/*
 * Computes all the parameters for each tick
 */
void ADnote::computecurrentparameters()
{
    float voicefreq, voicepitch, filterpitch, filterfreq, FMfreq,
          FMrelativepitch, globalfilterpitch;
    const float globalpitch = 0.01f * (FreqEnvelope->envout()
                           + NoteGlobalPar.FreqLfo->lfoout()
                           * ctl.modwheel.relmod);
    oldamplitude = newamplitude;
    newamplitude = volume * AmpEnvelope->envout_dB()
                         * NoteGlobalPar.AmpLfo->amplfoout();

    globalfilterpitch = FilterEnvelope->envout()
                        + NoteGlobalPar.FilterLfo->lfoout()
                        + FilterCenterPitch;

    float tmpfilterfreq = globalfilterpitch + ctl.filtercutoff.relfreq
                          + FilterFreqTracking;

    tmpfilterfreq = Filter::getrealfreq(tmpfilterfreq);

    float globalfilterq = FilterQ * ctl.filterq.relq;
    GlobalFilterL->setfreq_and_q(tmpfilterfreq, globalfilterq);
    if(stereo)
        GlobalFilterR->setfreq_and_q(tmpfilterfreq, globalfilterq);

    //compute the portamento, if it is used by this note
    const float portamentofreqrap = getPortamento(portamento);

    //compute parameters for all voices
    for(int nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
        auto &nvp = NoteVoicePar[nvoice];
        auto &nvi = NoteVoiceInternals[nvoice];
        if(!nvp.Enabled)
            continue;
        nvp.DelayTicks -= 1;
        if(nvp.DelayTicks > 0)
            continue;

        compute_unison_freq_rap(nvoice);

        /*******************/
        /* Voice Amplitude */
        /*******************/
        nvi.oldamplitude = nvi.newamplitude;
        nvi.newamplitude = 1.0f;

        if(nvp.AmpEnvelope)
            nvi.newamplitude *= nvp.AmpEnvelope->envout_dB();

        if(nvp.AmpLfo)
            nvi.newamplitude *= nvp.AmpLfo->amplfoout();

        /****************/
        /* Voice Filter */
        /****************/
        if(nvp.VoiceFilterL) {
            filterpitch = nvp.FilterCenterPitch;

            if(nvp.FilterEnvelope)
                filterpitch += nvp.FilterEnvelope->envout();

            if(nvp.FilterLfo)
                filterpitch += nvp.FilterLfo->lfoout();

            filterfreq = filterpitch + nvp.FilterFreqTracking;
            filterfreq = Filter::getrealfreq(filterfreq);

            nvp.VoiceFilterL->setfreq(filterfreq);
            if(stereo && nvp.VoiceFilterR)
                nvp.VoiceFilterR->setfreq(filterfreq);
        }

        if(NoteVoicePar[nvoice].noisetype == 0) { //compute only if the voice isn't noise
            /*******************/
            /* Voice Frequency */
            /*******************/
            voicepitch = 0.0f;
            if(nvp.FreqLfo)
                voicepitch += nvp.FreqLfo->lfoout() / 100.0f
                              * ctl.bandwidth.relbw;

            if(nvp.FreqEnvelope)
                voicepitch += nvp.FreqEnvelope->envout()
                              / 100.0f;
            voicefreq = getvoicebasefreq(nvoice)
                        * powf(2, (voicepitch + globalpitch) / 12.0f);
            voicefreq *= ctl.pitchwheel.relfreq; //change the frequency by the controller
            setfreq(nvoice, voicefreq * portamentofreqrap);

            /***************/
            /*  Modulator */
            /***************/
            if(nvp.FMEnabled != NONE) {
                FMrelativepitch = nvp.FMDetune / 100.0f;
                if(nvp.FMFreqEnvelope)
                    FMrelativepitch += nvp.FMFreqEnvelope->envout() / 100;
                FMfreq = powf(2.0f, FMrelativepitch
                         / 12.0f) * voicefreq * portamentofreqrap;
                setfreqFM(nvoice, FMfreq);

                nvi.FMoldamplitude = nvi.FMnewamplitude;
                nvi.FMnewamplitude = nvp.FMVolume * ctl.fmamp.relamp;
                if(nvp.FMAmpEnvelope)
                    nvi.FMnewamplitude *= nvp.FMAmpEnvelope->envout_dB();
            }
        }
    }
    time += synth->buffersize_f / synth->samplerate_f;
}


/*
 * Computes the Oscillator (Without Modulation) - LinearInterpolation
 */

/* As the code here is a bit odd due to optimization, here is what happens
 * First the current possition and frequency are retrieved from the running
 * state. These are broken up into high and low portions to indicate how many
 * samples are skipped in one step and how many fractional samples are skipped.
 * Outside of this method the fractional samples are just handled with floating
 * point code, but that's a bit slower than it needs to be. In this code the low
 * portions are known to exist between 0.0 and 1.0 and it is known that they are
 * stored in single precision floating point IEEE numbers. This implies that
 * a maximum of 24 bits are significant. The below code does your standard
 * linear interpolation that you'll see throughout this codebase, but by
 * sticking to integers for tracking the overflow of the low portion, around 15%
 * of the execution time was shaved off in the ADnote test.
 */
inline void ADnote::ComputeVoiceOscillator_LinearInterpolation(int nvoice)
{
    auto &nvi = NoteVoiceInternals[nvoice];
    for(int k = 0; k < nvi.unison_size; ++k) {
        int    poshi  = nvi.oscposhi[k];
        int    poslo  = nvi.oscposlo[k] * (1<<24);
        int    freqhi = nvi.oscfreqhi[k];
        int    freqlo = nvi.oscfreqlo[k] * (1<<24);
        float *smps   = NoteVoicePar[nvoice].OscilSmp;
        float *tw     = tmpwave_unison[k];
        assert(nvi.oscfreqlo[k] < 1.0f);
        for(int i = 0; i < synth->buffersize; ++i) {
            tw[i]  = (smps[poshi] * ((1<<24) - poslo) + smps[poshi + 1] * poslo)/(1.0f*(1<<24));
            poslo += freqlo;
            poshi += freqhi + (poslo>>24);
            poslo &= 0xffffff;
            poshi &= oscSize - 1;
        }
        nvi.oscposhi[k] = poshi;
        nvi.oscposlo[k] = poslo/(1.0f*(1<<24));
    }
}



/*
 * Computes the Oscillator (Morphing)
 */
inline void ADnote::ComputeVoiceOscillatorMorph(int nvoice)
{
    auto &nvi = NoteVoiceInternals[nvoice];
    auto &nvp = NoteVoicePar[nvoice];
    float amp;
    ComputeVoiceOscillator_LinearInterpolation(nvoice);
    if(nvi.FMnewamplitude > 1.0f)
        nvi.FMnewamplitude = 1.0f;
    if(nvi.FMoldamplitude > 1.0f)
        nvi.FMoldamplitude = 1.0f;

    if(NoteVoicePar[nvoice].FMVoice >= 0) {
        //if I use VoiceOut[] as modullator
        int FMVoice = NoteVoicePar[nvoice].FMVoice;
        for(int k = 0; k < nvi.unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            for(int i = 0; i < synth->buffersize; ++i) {
                amp = INTERPOLATE_AMPLITUDE(nvi.FMoldamplitude,
                                            nvi.FMnewamplitude,
                                            i,
                                            synth->buffersize);
                tw[i] = tw[i] * (1.0f - amp) + amp * NoteVoicePar[FMVoice].VoiceOut[i];
            }
        }
    }
    else
        for(int k = 0; k < nvi.unison_size; ++k) {
            int    poshiFM  = nvi.oscposhiFM[k];
            float  posloFM  = nvi.oscposloFM[k];
            int    freqhiFM = nvi.oscfreqhiFM[k];
            float  freqloFM = nvi.oscfreqloFM[k];
            float *tw = tmpwave_unison[k];

            for(int i = 0; i < synth->buffersize; ++i) {
                amp = INTERPOLATE_AMPLITUDE(nvi.FMoldamplitude,
                                            nvi.FMnewamplitude,
                                            i,
                                            synth->buffersize);
                tw[i] = tw[i] * (1.0f - amp) + amp
                        * (nvp.FMSmp[poshiFM] * (1 - posloFM)
                           + nvp.FMSmp[poshiFM + 1] * posloFM);
                posloFM += freqloFM;
                if(posloFM >= 1.0f) {
                    posloFM -= 1.0f;
                    poshiFM++;
                }
                poshiFM += freqhiFM;
                poshiFM &= oscSize - 1;
            }
            nvi.oscposhiFM[k] = poshiFM;
            nvi.oscposloFM[k] = posloFM;
        }
}

/*
 * Computes the Oscillator (Ring Modulation)
 */
inline void ADnote::ComputeVoiceOscillatorRingModulation(int nvoice)
{
    auto &nvi = NoteVoiceInternals[nvoice];
    auto &nvp = NoteVoicePar[nvoice];
    float amp;
    ComputeVoiceOscillator_LinearInterpolation(nvoice);
    if(nvi.FMnewamplitude > 1.0f)
        nvi.FMnewamplitude = 1.0f;
    if(nvi.FMoldamplitude > 1.0f)
        nvi.FMoldamplitude = 1.0f;
    if(NoteVoicePar[nvoice].FMVoice >= 0)
        // if I use VoiceOut[] as modullator
        for(int k = 0; k < nvi.unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            for(int i = 0; i < synth->buffersize; ++i) {
                amp = INTERPOLATE_AMPLITUDE(nvi.FMoldamplitude,
                                            nvi.FMnewamplitude,
                                            i,
                                            synth->buffersize);
                int FMVoice = NoteVoicePar[nvoice].FMVoice;
                tw[i] *= (1.0f - amp) + amp * NoteVoicePar[FMVoice].VoiceOut[i];
            }
        }
    else
        for(int k = 0; k < nvi.unison_size; ++k) {
            int    poshiFM  = nvi.oscposhiFM[k];
            float  posloFM  = nvi.oscposloFM[k];
            int    freqhiFM = nvi.oscfreqhiFM[k];
            float  freqloFM = nvi.oscfreqloFM[k];
            float *tw = tmpwave_unison[k];

            for(int i = 0; i < synth->buffersize; ++i) {
                amp = INTERPOLATE_AMPLITUDE(nvi.FMoldamplitude,
                                            nvi.FMnewamplitude,
                                            i,
                                            synth->buffersize);
                tw[i] *= (nvp.FMSmp[poshiFM] * (1.0f - posloFM)
                          + nvp.FMSmp[poshiFM + 1] * posloFM) * amp
                         + (1.0f - amp);
                posloFM += freqloFM;
                if(posloFM >= 1.0f) {
                    posloFM -= 1.0f;
                    poshiFM++;
                }
                poshiFM += freqhiFM;
                poshiFM &= oscSize - 1;
            }
            nvi.oscposhiFM[k] = poshiFM;
            nvi.oscposloFM[k] = posloFM;
        }
}



/*
 * Computes the Oscillator (Phase Modulation or Frequency Modulation)
 */
inline void ADnote::ComputeVoiceOscillatorFrequencyModulation(int nvoice,
                                                              int FMmode)
{
    auto &nvi = NoteVoiceInternals[nvoice];
    auto &nvp = NoteVoicePar[nvoice];
    int   carposhi = 0;
    int   FMmodfreqhi = 0;
    float FMmodfreqlo = 0, carposlo = 0;

    if(NoteVoicePar[nvoice].FMVoice >= 0)
        //if I use VoiceOut[] as modulator
        for(int k = 0; k < nvi.unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            memcpy(tw, NoteVoicePar[NoteVoicePar[nvoice].FMVoice].VoiceOut,
                   synth->bufferbytes);
        }
    else
        //Compute the modulator and store it in tmpwave_unison[][]
        for(int k = 0; k < nvi.unison_size; ++k) {
            int    poshiFM  = nvi.oscposhiFM[k];
            float  posloFM  = nvi.oscposloFM[k];
            int    freqhiFM = nvi.oscfreqhiFM[k];
            float  freqloFM = nvi.oscfreqloFM[k];
            float *tw = tmpwave_unison[k];

            for(int i = 0; i < synth->buffersize; ++i) {
                tw[i] =
                    (NoteVoicePar[nvoice].FMSmp[poshiFM] * (1.0f - posloFM)
                     + NoteVoicePar[nvoice].FMSmp[poshiFM + 1] * posloFM);
                posloFM += freqloFM;
                if(posloFM >= 1.0f) {
                    posloFM = fmod(posloFM, 1.0f);
                    poshiFM++;
                }
                poshiFM += freqhiFM;
                poshiFM &= oscSize - 1;
            }
            nvi.oscposhiFM[k] = poshiFM;
            nvi.oscposloFM[k] = posloFM;
        }
    // Amplitude interpolation
    if(ABOVE_AMPLITUDE_THRESHOLD(nvi.FMoldamplitude,
                                 nvi.FMnewamplitude))
        for(int k = 0; k < nvi.unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            for(int i = 0; i < synth->buffersize; ++i)
                tw[i] *= INTERPOLATE_AMPLITUDE(nvi.FMoldamplitude,
                                               nvi.FMnewamplitude,
                                               i,
                                               synth->buffersize);
        }
    else
        for(int k = 0; k < nvi.unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            for(int i = 0; i < synth->buffersize; ++i)
                tw[i] *= nvi.FMnewamplitude;
        }


    //normalize: makes all sample-rates, oscil_sizes to produce same sound
    if(FMmode != 0) { //Frequency modulation
        float normalize = oscSize / 262144.0f * 44100.0f
                          / synth->samplerate_f;
        for(int k = 0; k < nvi.unison_size; ++k) {
            float *tw    = tmpwave_unison[k];
            float  fmold = nvi.FMoldsmp[k];
            for(int i = 0; i < synth->buffersize; ++i) {
                fmold = fmod(fmold + tw[i] * normalize, oscSize);
                tw[i] = fmold;
            }
            nvi.FMoldsmp[k] = fmold;
        }
    }
    else {  //Phase modulation
        float normalize = oscSize / 262144.0f;
        for(int k = 0; k < nvi.unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            for(int i = 0; i < synth->buffersize; ++i)
                tw[i] *= normalize;
        }
    }

    //do the modulation
    for(int k = 0; k < nvi.unison_size; ++k) {
        float *tw     = tmpwave_unison[k];
        int    poshi  = nvi.oscposhi[k];
        float  poslo  = nvi.oscposlo[k];
        int    freqhi = nvi.oscfreqhi[k];
        float  freqlo = nvi.oscfreqlo[k];

        for(int i = 0; i < synth->buffersize; ++i) {
            F2I(tw[i], FMmodfreqhi);
            FMmodfreqlo = fmod(tw[i] + 0.0000000001f, 1.0f);
            if(FMmodfreqhi < 0)
                FMmodfreqlo++;

            //carrier
            carposhi = poshi + FMmodfreqhi;
            carposlo = poslo + FMmodfreqlo;

            if(carposlo >= 1.0f) {
                carposhi++;
                carposlo = fmod(carposlo, 1.0f);
            }
            carposhi &= (oscSize - 1);

            tw[i] = nvp.OscilSmp[carposhi] * (1.0f - carposlo)
                    + nvp.OscilSmp[carposhi + 1] * carposlo;

            poslo += freqlo;
            if(poslo >= 1.0f) {
                poslo = fmod(poslo, 1.0f);
                poshi++;
            }

            poshi += freqhi;
            poshi &= oscSize - 1;
        }
        nvi.oscposhi[k] = poshi;
        nvi.oscposlo[k] = poslo;
    }
}

/*
 * Computes the Noise
 */
inline void ADnote::ComputeVoiceNoise(int nvoice)
{
    for(int k = 0; k < NoteVoiceInternals[nvoice].unison_size; ++k) {
        float *tw = tmpwave_unison[k];
        for(int i = 0; i < synth->buffersize; ++i)
            tw[i] = RND * 2.0f - 1.0f;
    }
}



/*
 * Compute the ADnote samples
 * Returns 0 if the note is finished
 */
int ADnote::noteout(float *outl, float *outr)
{
    memcpy(outl, denormalkillbuf, synth->bufferbytes);
    memcpy(outr, denormalkillbuf, synth->bufferbytes);

    if(!NoteEnabled)
        return 0;

    memset(bypassl, 0, synth->bufferbytes);
    memset(bypassr, 0, synth->bufferbytes);
    computecurrentparameters();

    for(unsigned nvoice = 0; nvoice < NUM_VOICES; ++nvoice) {
        const auto &nvp = NoteVoicePar[nvoice];
        auto &nvi = NoteVoiceInternals[nvoice];
        if(!nvp.Enabled || nvp.DelayTicks > 0)
            continue;
        if(nvp.noisetype == 0) //voice mode=sound
            switch(nvp.FMEnabled) {
                case MORPH:
                    ComputeVoiceOscillatorMorph(nvoice);
                    break;
                case RING_MOD:
                    ComputeVoiceOscillatorRingModulation(nvoice);
                    break;
                case PHASE_MOD:
                    ComputeVoiceOscillatorFrequencyModulation(nvoice, 0);
                    break;
                case FREQ_MOD:
                    ComputeVoiceOscillatorFrequencyModulation(nvoice, 1);
                    break;
                default:
                    ComputeVoiceOscillator_LinearInterpolation(nvoice);
            }
        else
            ComputeVoiceNoise(nvoice);
        // Voice Processing


        //mix subvoices into voice
        memset(tmpwavel, 0, synth->bufferbytes);
        if(stereo)
            memset(tmpwaver, 0, synth->bufferbytes);
        for(int k = 0; k < nvi.unison_size; ++k) {
            float *tw = tmpwave_unison[k];
            if(stereo) {
                float stereo_pos = 0;
                if(nvi.unison_size > 1)
                    stereo_pos = k / (float)(nvi.unison_size - 1) * 2.0f - 1.0f;
                const float stereo_spread = nvi.unison_stereo_spread * 2.0f; //between 0 and 2.0f
                if(stereo_spread > 1.0f) {
                    float stereo_pos_1 = (stereo_pos >= 0.0f) ? 1.0f : -1.0f;
                    stereo_pos =
                        (2.0f - stereo_spread) * stereo_pos
                        + (stereo_spread - 1.0f) * stereo_pos_1;
                }
                else
                    stereo_pos *= stereo_spread;

                if(nvi.unison_size == 1)
                    stereo_pos = 0.0f;
                float panning = (stereo_pos + 1.0f) * 0.5f;


                float lvol = limit((1.0f - panning) * 2.0f, 0.0f, 1.0f);
                float rvol = limit(panning * 2.0f,          0.0f, 1.0f);

                if(nvi.unison_invert_phase[k]) {
                    lvol = -lvol;
                    rvol = -rvol;
                }

                for(int i = 0; i < synth->buffersize; ++i)
                    tmpwavel[i] += tw[i] * lvol;
                for(int i = 0; i < synth->buffersize; ++i)
                    tmpwaver[i] += tw[i] * rvol;
            }
            else
                for(int i = 0; i < synth->buffersize; ++i)
                    tmpwavel[i] += tw[i];
        }

        //reduce the amplitude for large unison sizes
        const float unison_amplitude = 1.0f / sqrt(nvi.unison_size);

        // Amplitude
        applyAmp(tmpwavel, stereo ? tmpwaver : NULL,
                nvi.oldamplitude * unison_amplitude,
                nvi.newamplitude * unison_amplitude);

        // Fade in
        if(nvi.firsttick) {
            fadein(&tmpwavel[0]);
            if(stereo)
                fadein(&tmpwaver[0]);
            nvi.firsttick = false;
        }


        // Filter
        if(nvp.VoiceFilterL)
            nvp.VoiceFilterL->filterout(&tmpwavel[0]);
        if(stereo && nvp.VoiceFilterR)
            nvp.VoiceFilterR->filterout(&tmpwaver[0]);

        //Fade out when envelope is finished
        if(nvp.AmpEnvelope && nvp.AmpEnvelope->finished()) {
            fadeout(tmpwavel);
            if(stereo)
                fadeout(tmpwaver);
        }
        //the voice is killed later


        // Put the ADnote samples in VoiceOut
        // (without appling Global volume, because I wish to use this voice as a modullator)
        if(nvp.VoiceOut) {
            if(stereo)
                for(int i = 0; i < synth->buffersize; ++i)
                    nvp.VoiceOut[i] = tmpwavel[i] + tmpwaver[i];
            else   //mono
                for(int i = 0; i < synth->buffersize; ++i)
                    nvp.VoiceOut[i] = tmpwavel[i];
        }


        // Add the voice to either the bypass channel or output
        const float panl = nvp.Panning * 2.0f;
        const float panr = (1.0f - nvp.Panning) * 2.0f;
        float *bufl = nvp.filterbypass ? bypassl : outl;
        float *bufr = nvp.filterbypass ? bypassr : outr;
        if(stereo)
            for(int i = 0; i < synth->buffersize; ++i) {
                bufl[i] += tmpwavel[i] * nvp.Volume * panl;
                bufr[i] += tmpwaver[i] * nvp.Volume * panr;
            }
        else
            for(int i = 0; i < synth->buffersize; ++i)
                bufl[i] += tmpwavel[i] * nvp.Volume;

        // chech if there is necesary to proces the voice longer
        // (if the Amplitude envelope isn't finished)
        if(nvp.AmpEnvelope && nvp.AmpEnvelope->finished())
            KillVoice(nvoice);
    }


    //Processing Global parameters
    GlobalFilterL->filterout(&outl[0]);

    if(stereo == 0) { //set the right channel=left channel
        memcpy(outr, outl, synth->bufferbytes);
        memcpy(bypassr, bypassl, synth->bufferbytes);
    } else
        GlobalFilterR->filterout(&outr[0]);

    for(int i = 0; i < synth->buffersize; ++i) {
        outl[i] += bypassl[i];
        outr[i] += bypassr[i];
    }

    //Apply the punch
    applyPunch(outl, outr, NoteGlobalPar.Punch);
    
    applyPanning(outl, outr,
            oldamplitude, newamplitude,
            panning, (1.0f - panning));


    // Apply legato-specific sound signal modifications
    legato.apply(*this, outl, outr);


    // Check if the global amplitude is finished.
    // If it does, disable the note
    if(AmpEnvelope->finished()) {
        fadeout(outl);
        fadeout(outr);
        KillNote();
    }
    return 1;
}


/*
 * Relase the key (NoteOff)
 */
void ADnote::relasekey()
{
    for(int nvoice = 0; nvoice < NUM_VOICES; ++nvoice)
        NoteVoicePar[nvoice].releasekey();
    FreqEnvelope->relasekey();
    FilterEnvelope->relasekey();
    AmpEnvelope->relasekey();
}

/*
 * Check if the note is finished
 */
bool ADnote::finished() const
{
    return !NoteEnabled;
}

void ADnote::Voice::releasekey()
{
    if(!Enabled)
        return;
    if(AmpEnvelope)
        AmpEnvelope->relasekey();
    if(FreqEnvelope)
        FreqEnvelope->relasekey();
    if(FilterEnvelope)
        FilterEnvelope->relasekey();
    if(FMFreqEnvelope)
        FMFreqEnvelope->relasekey();
    if(FMAmpEnvelope)
        FMAmpEnvelope->relasekey();
}

void ADnote::Voice::kill(Allocator &memory)
{
    memory.devalloc(OscilSmp);
    memory.dealloc(FreqEnvelope);
    memory.dealloc(FreqLfo);
    memory.dealloc(AmpEnvelope);
    memory.dealloc(AmpLfo);
    memory.dealloc(VoiceFilterL);
    memory.dealloc(VoiceFilterR);
    memory.dealloc(FilterEnvelope);
    memory.dealloc(FilterLfo);
    memory.dealloc(FMFreqEnvelope);
    memory.dealloc(FMAmpEnvelope);

    if(FMEnabled != NONE && FMVoice < 0)
        memory.devalloc(FMSmp);

    if(VoiceOut)
        memset(VoiceOut, 0, synth->bufferbytes);
    //do not delete, yet: perhaps is used by another voice

    Enabled = false;
}

void ADnote::Global::kill(Allocator &memory)
{
    memory.dealloc(FreqLfo);
    memory.dealloc(AmpLfo);
    memory.dealloc(FilterLfo);
}

void ADnote::Global::initparameters(const ADnoteGlobalParam &param,
                                    class Allocator &memory,
                                    float basefreq, float velocity,
                                    bool stereo)
{
    FreqLfo      = memory.alloc<LFO>(param.FreqLfo, basefreq);
    AmpLfo      = memory.alloc<LFO>(param.AmpLfo, basefreq);
    FilterLfo      = memory.alloc<LFO>(param.FilterLfo, basefreq);
}
