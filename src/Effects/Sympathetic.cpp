/*
  ZynAddSubFX - a software synthesizer

  Sympathetic.cpp - Distorsion effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "Sympathetic.h"
#include "../DSP/AnalogFilter.h"
#include "CombFilterBank.h"
#include "../Misc/Allocator.h"
#include <cmath>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include "../globals.h"

#define INH_MAX_OCTAVES 6.0f

namespace zyn {

#define rObject Sympathetic
#define rBegin [](const char *msg, rtosc::RtData &d) {
#define rEnd }

#define rEffParRange(name, idx, ...) \
  {STRINGIFY(name) "::i",  rProp(parameter) rDefaultDepends(preset) \
      DOC(__VA_ARGS__), NULL, rEffParCb(idx)}

rtosc::Ports Sympathetic::ports = {
    {"preset::i", rProp(parameter)
                  rOptions(Generic, Piano, Grand, Guitar, 12-String, PitchDrop)
                  rProp(alias)
                  rDefault(0)
                  rDoc("Instrument Presets"), 0,
                  rBegin;
                  rObject *o = (rObject*)d.obj;
                  if(rtosc_narguments(msg))
                      o->setpreset(rtosc_argument(msg, 0).i);
                  else
                      d.reply(d.loc, "i", o->Ppreset);
                  rEnd},
    {"presetOfVolume:", rProp(internal), 0,
        rBegin; (void)msg;
        rObject *o = (rObject*)d.obj;
        d.reply(d.loc, "i", o->Ppreset + (16 * o->insertion));
        rEnd},
    rEffParVol(rDefault(127) rDefaultDepends(presetOfVolume),
            rPresetsAt(0, 84, 66, 53, 66, 60),
            rPresetsAt(16, 127, 100, 80, 100, 90)),
    rEffParPan(rDefault(64)),
    rEffPar(Pq, 2, rShort("q"), rDefault(65),
            rPresets(125, 125, 125, 110, 110, 110), "Resonance"),
    rEffPar(Pdrive,   3, rShort("dr"), rDefault(65),
            rPresets(5, 5, 5, 20, 20, 20), "Input Amplification"),
    rEffPar(Plevel,   4, rShort("lev"), rDefault(65),
            rPresets(80, 80, 90, 65, 77, 77), "Output Amplification"),
    rEffPar(Punison_frequency_spread,  5, rShort("detune"), rDefault(30),
            rPresets(10, 10, 5, 0, 10, 0), "Unison String Detune"),
    rEffParTF(Pnegate, 6, rShort("neg"), rDefault(false), "Negate Signal"),
    rEffPar(Plpf, 7, rShort("lpf"), rDefault(127), "Low Pass Cutoff"),
    rEffPar(Phpf, 8, rShort("hpf"), rDefault(0), "High Pass Cutoff"),
    rEffParRange(Punison_size, 9, rShort("uni"), rLinear(1,3), rDefault(1),
            rPresets(3, 3, 1, 1, 2, 1), "Number of Unison Strings"),
    rEffParRange(Pstrings, 10, rShort("str"), rLinear(0,76), rDefault(12),
            rPresets(12, 12, 60, 6, 6, 8), "Number of Strings"),
    rEffPar(Pbasenote, 11, rShort("base"), rDefault(57), // basefreq = powf(2.0f, (basenote-69)/12)*440; 57->220Hz
            rPresets(57, 57, 33, 52, 52, 52), "Midi Note of Lowest String"),
    rEffPar(PdropRate, 12, rShort("dr"), rDefault(64),
            rPresets(64, 64, 64, 64, 64, 32), "Pitch Drop Rate"),
    rEffPar(PmaxDrop, 13, rShort("max"), rDefault(64),
            rPresets(0, 0, 0, 0, 0, 32),  "Max Drop"),
    rEffPar(PfadingTime, 14, rShort("fade"), rDefault(5),
            rPresets(0, 0, 0, 0, 0, 32),  "Fading Time"),
    rEffPar(PfreqOffset, 15, rShort("offset"), rDefault(64), "offset"),
    rEffPar(Pinharmonicity, 16, rShort("inharmonicity"), rDefault(0), "inharmonicity"),
    rEffPar(Pbeta, 17, rShort("beta"), rDefault(64), "beta"),
    rEffPar(Pgamma, 18, rShort("gamma"), rDefault(64), "gamma"),
    rEffPar(PcontactDist, 19, rShort("contact"), rDefault(64), "contact disctance"),
    rEffPar(PcontactStrength, 20, rShort("strength"), rDefault(0), "contact strength"),
    rArrayF(freqs, 88, rLinear(27.50f,4186.01f),
           "String Frequencies"),
};

#undef rBegin
#undef rEnd
#undef rObject

Sympathetic::Sympathetic(EffectParams pars)
    :Effect(pars),
      Pvolume(127),
      Pdrive(65),
      Plevel(65),
      Ptype(0),
      Pnegate(0),
      Plpf(127),
      Phpf(0),
      Pstereo(0),
      Pq(65),
      Punison_size(1),
      Punison_frequency_spread(30),
      Pstrings(12),
      Pbasenote(57),
      PdropRate(64),
      PmaxDrop(0),
      PfadingTime(96),
      PfreqOffset(64),
      baseFreq(220.0f),
      srate(pars.srate)
{
    lpfl = memory.alloc<AnalogFilter>(2, 22000, 1, 0, pars.srate, pars.bufsize);
    lpfr = memory.alloc<AnalogFilter>(2, 22000, 1, 0, pars.srate, pars.bufsize);
    hpfl = memory.alloc<AnalogFilter>(3, 20, 1, 0, pars.srate, pars.bufsize);
    hpfr = memory.alloc<AnalogFilter>(3, 20, 1, 0, pars.srate, pars.bufsize);

    // precalc gainbwd_init = gainbwd_offset + gainbwd_factor * Pq
    // 0.873f + 0.001f * 65 = 0.873f + 0.065f = 0.938f
    filterBank = memory.alloc<CombFilterBank>(&memory, pars.srate, pars.bufsize, 0.938f);

    setpreset(Ppreset);
    calcFreqs(); // sets freqs
    cleanup();
}

Sympathetic::~Sympathetic()
{
    memory.dealloc(lpfl);
    memory.dealloc(lpfr);
    memory.dealloc(hpfl);
    memory.dealloc(hpfr);
    memory.dealloc(filterBank);
}

//Cleanup the effect
void Sympathetic::cleanup(void)
{
    lpfl->cleanup();
    hpfl->cleanup();
    lpfr->cleanup();
    hpfr->cleanup();
}


//Apply the filters
void Sympathetic::applyfilters(float *efxoutl, float *efxoutr)
{
    if(Plpf!=127) lpfl->filterout(efxoutl);
    if(Phpf!=0) hpfl->filterout(efxoutl);
    if(Pstereo != 0) { //stereo
        if(Plpf!=127) lpfr->filterout(efxoutr);
        if(Phpf!=0) hpfr->filterout(efxoutr);
    }
}


//Effect output
void Sympathetic::out(const Stereo<float *> &smp)
{
    float inputvol = powf(2.0f, (Pdrive - 65.0f) / 128.0f) / 2.0f;
    if(Pnegate)
        inputvol *= -1.0f;

    if(Pstereo) //Stereo
        for(int i = 0; i < buffersize; ++i) {
            efxoutl[i] = smp.l[i] * inputvol * pangainL;
            efxoutr[i] = smp.r[i] * inputvol * pangainR;
        }
    else //Mono
        for(int i = 0; i < buffersize; ++i)
            efxoutl[i] = (smp.l[i] * pangainL + smp.r[i] * pangainR) * inputvol;

    filterBank->filterout(efxoutl);
    if(Pstereo)
        filterBank->filterout(efxoutr);

    applyfilters(efxoutl, efxoutr);

    if(!Pstereo)
        memcpy(efxoutr, efxoutl, bufferbytes);

    float level = dB2rap(60.0f * Plevel / 127.0f - 40.0f);
    for(int i = 0; i < buffersize; ++i) {
        float lout = efxoutl[i];
        float rout = efxoutr[i];
        float l    = lout * (1.0f - lrcross) + rout * lrcross;
        float r    = rout * (1.0f - lrcross) + lout * lrcross;
        lout = l;
        rout = r;

        efxoutl[i] = lout * 2.0f * level;
        efxoutr[i] = rout * 2.0f * level;
    }
}

//Parameter control
void Sympathetic::setvolume(unsigned char _Pvolume)
{
    Pvolume = _Pvolume;

    if(insertion == 0) {
        outvolume = powf(0.01f, (1.0f - Pvolume / 127.0f)) * 4.0f;
        volume    = 1.0f;
    }
    else
        volume = outvolume = Pvolume / 127.0f;
    if(Pvolume == 0)
        cleanup();
}

void Sympathetic::setlpf(unsigned char _Plpf)
{
    Plpf = _Plpf;
    float fr = expf(sqrtf(Plpf / 127.0f) * logf(25000.0f)) + 40.0f;
    lpfl->setfreq(fr);
    lpfr->setfreq(fr);
}

void Sympathetic::sethpf(unsigned char _Phpf)
{
    Phpf = _Phpf;
    float fr = expf(sqrtf(Phpf / 127.0f) * logf(25000.0f)) + 20.0f;
    hpfl->setfreq(fr);
    hpfr->setfreq(fr);
}

void Sympathetic::calcFreqs()
{
    switch(Ppreset) {
        case 0:
            calcFreqsGeneric();
            break;
        case 1:
        case 2:
            calcFreqsPiano();
            break;
        case 3:
        case 4:
            calcFreqsGuitar();
            break;
        case 5:
            calcFreqsPitchDrop();
            break;
    }
}


void Sympathetic::calcFreqsPitchDrop()
{
    Pstrings = 8;
    for(unsigned int i = 0; i < Pstrings; i++)
    {
        filterBank->delays[i] = ((float)samplerate) * freeverb_freqs[i] / 44100.0f;
    }
    //          factor for pitchdrop Maxdrop: 2^4 = 16, Pitchoffset: 2  -> 32.0f
    const unsigned int mem_size_new = (int)ceilf(( filterBank->delays[0] * 32.0f * 1.03f + buffersize + 2)/16) * 16;

    filterBank->setStrings(Pstrings,mem_size_new);
}

void Sympathetic::calcFreqsGeneric()
{
    const float unison_spread_semicent = powf(Punison_frequency_spread / 63.5f, 2.0f) * 25.0f;
    const float unison_real_spread_up   = powf(2.0f, (unison_spread_semicent * 0.5f) / 1200.0f);
    const float unison_real_spread_down = 1.0f / unison_real_spread_up;

    const float min_freq = baseFreq * 0.125f; // Prevent overly long delays (3 octaves down)

    for (unsigned int i = 0; i < Punison_size * Pstrings; i += Punison_size)
    {
        // normalized index along the string set (0..1)
        const float n_norm = (float)(i / Punison_size) / (float)(Pstrings - 1);
        const float n = (float)(i / Punison_size); // integer index for semitone spacing

        // --- Base linear distribution: exact semitone spacing ---
        // Each "string" corresponds to one semitone relative to baseFreq
        float log2Base = n / 12.0f; // 12 semitones per octave

        // --- Nonlinear deformation controlled by beta ---
        // When beta = 1.0 → no deformation (pure semitone spacing)
        // beta > 1.0  → high partials are stretched (metallic)
        // beta < 1.0  → low partials are stretched (softer, string-like)
        float nonlinearDeform = 0.0f;
        if (beta != 1.0f && n_norm > 0.0f) {
            // power curve relative to linear spacing
            // result normalized so that the end of the range stays at INH_MAX_OCTAVES
            float scaled = powf(n_norm, beta) - n_norm;
            nonlinearDeform = scaled * (INH_MAX_OCTAVES / 12.0f); // scale to octave range
        }

        // --- Inharmonic Offset Calculation ---
        // Goal:
        //   Add inharmonicity *above* the regular harmonic (log2-based) distribution,
        //   so that the parameter acts independently from the harmonic stretch (beta).
        //   The idea is: 'beta' shapes the overall spacing of harmonics (macro scale),
        //   while 'inharmonicity' adds local deviations (micro scale) mainly in upper regions.

        float inharmonicOffset = 0.0f;

        if (inharmonicity > 0.0f) {
            // Nonlinear weighting that emphasizes higher string indices
            // n_norm = 0 -> lowest string, n_norm = 1 -> highest string
            //
            // Using powf(n_norm, gamma) ensures we can shape where the inharmonicity starts:
            //   gamma < 1.0 → affects more of the range (softer curve)
            //   gamma > 1.0 → concentrated toward the top (steeper curve)
            //
            // The (1 - powf(...)) term makes the offset start at 0 for n_norm=0
            // and smoothly reach its maximum near n_norm=1.

            float shaped = powf(n_norm, gamma);           // shape curve along strings
            float rising = 1.0f - powf(1.0f - shaped, 2); // emphasize top-end effect

            // Apply inharmonic scaling and clamp to maximum range
            // INH_MAX_OCTAVES defines the maximum allowed offset range (e.g., 2 = 2 octaves)
            inharmonicOffset = inharmonicity * rising * INH_MAX_OCTAVES;
        }

        // Combine base (halftone) + deformation + inharmonicity
        float log2Offset = log2Base + nonlinearDeform + inharmonicOffset;

        float centerFreq = baseFreq * powf(2.0f, log2Offset);

        // Skip frequencies that are too low
        if (centerFreq < min_freq) {
            filterBank->delays[i] = 0.0f;
            if (Punison_size > 1) filterBank->delays[i + 1] = 0.0f;
            if (Punison_size > 2) filterBank->delays[i + 2] = 0.0f;
            continue;
        }


        // Calculate delays for valid frequencies
        filterBank->delays[i] = ((float)samplerate)/centerFreq;
        printf("\ndelays[%d]: %f", i, filterBank->delays[i]);
        if (Punison_size > 1) {
            filterBank->delays[i+1] = ((float)samplerate)/(centerFreq * unison_real_spread_up);
            printf(" [%d]: %f", i+1, filterBank->delays[i+1]);
        }
        if (Punison_size > 2) {
            filterBank->delays[i+2] = ((float)samplerate)/(centerFreq * unison_real_spread_down);
            printf("[%d]: %f", i+2, filterBank->delays[i+2]);
        }
    }
    // factor for pitchdrop Maxdrop: 2^4 = 16, Pitchoffset: 2^1 = 2  -> 2^5 = 32.0f
    filterBank->setStrings(Pstrings*Punison_size, baseFreq/32.0f);
}

void Sympathetic::calcFreqsPiano()
{
    const float unison_spread_semicent = powf(Punison_frequency_spread / 63.5f, 2.0f) * 25.0f;
    const float unison_real_spread_up = powf(2.0f, (unison_spread_semicent * 0.5f) / 1200.0f);
    const float unison_real_spread_down = 1.0f/unison_real_spread_up;

    for(unsigned int i = 0; i < Punison_size*Pstrings; i+=Punison_size)
    {
        const float centerFreq = powf(2.0f, (float)i / 36.0f) * baseFreq;
        const unsigned int stringchoir_size =
            i>num_single_strings ? (i>Pstrings-num_triple_strings ? 3 : 2) :1;
        filterBank->delays[i] = ((float)samplerate)/centerFreq;
        if (stringchoir_size > 1) filterBank->delays[i+1] = ((float)samplerate)/(centerFreq * unison_real_spread_up);
        else filterBank->delays[i+1] = 0;
        if (stringchoir_size > 2) filterBank->delays[i+2] = ((float)samplerate)/(centerFreq * unison_real_spread_down);
        else filterBank->delays[i+2] = 0;
    }
    filterBank->setStrings(Pstrings*Punison_size,baseFreq);

}

void Sympathetic::calcFreqsGuitar()
{
    const float unison_spread_semicent = powf(Punison_frequency_spread / 63.5f, 2.0f) * 25.0f;
    const float unison_real_spread_up = powf(2.0f, (unison_spread_semicent * 0.5f) / 1200.0f);

    for(auto i = 0; i < 6*Punison_size; i+=Punison_size)
    {
        assert(guitar_freqs[i/Punison_size]>0.0f);
        filterBank->delays[i] = ((float)samplerate)/guitar_freqs[i/Punison_size];
        if (Punison_size > 1) filterBank->delays[i+1] = ((float)samplerate)/(guitar_freqs[i/Punison_size] * unison_real_spread_up);
    }
    filterBank->setStrings(Pstrings*Punison_size,guitar_freqs[0]);

}

unsigned char Sympathetic::getpresetpar(unsigned char npreset, unsigned int npar)
{
#define PRESET_SIZE 15
#define NUM_PRESETS 6
    static const unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        //Vol Pan Q Drive Lev Spr neg lp hp sz  strings note
        //Generic
        {127, 64, 125, 5, 80, 10, 0, 127, 0, 3,   12,  57, 64, 0, 0},
        //Piano 12-String
        {100, 64, 125, 5, 80, 10, 0, 127, 0, 3,   12,  57, 64, 0, 0},
        //Piano 60-String
        {80,  64, 125, 5, 90, 5,  0, 127, 0, 1,   60,  33, 64, 0, 0},
        //Guitar 6-String
        {100, 64, 110, 20, 65, 0,  0, 127, 0, 1,    6,  52, 64, 0, 0},
        //Guitar 12-String
        {90,  64, 110, 20, 77, 10, 0, 127, 0, 2,    6,  52, 64, 0, 0},
        //Pitchdrop
        {90,  64, 110, 20, 77, 10, 0, 127, 0, 2,    6,  52, 32, 32, 32},
    };
    if(npreset < NUM_PRESETS && npar < PRESET_SIZE) {
        if(npar == 0 && insertion == 0) {
            /* lower the volume if this is system effect */
            return (2 * presets[npreset][npar]) / 3;
        }
        return presets[npreset][npar];
    }
    else if (npreset < NUM_PRESETS && npar==15)
        return 64;

    return 0;
}

void Sympathetic::setpreset(unsigned char npreset)
{
    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n != 128; n++)
        changepar(n, getpresetpar(npreset, n));
    Ppreset = npreset;

    calcFreqs();
    cleanup();
}


void Sympathetic::changepar(int npar, unsigned char value)
{
    switch(npar) {
        case 0:
            setvolume(value);
            break;
        case 1:
            setpanning(value);
            break;
        case 2:
            Pq = value;
            filterBank->gainbwd = gainbwd_offset + (float)Pq * gainbwd_factor;
            break;
        case 3:
            Pdrive = value;
            filterBank->inputgain = (float)Pdrive/65.0f;
            break;
        case 4:
            Plevel = value;
            filterBank->outgain = (float)Plevel/65.0f;
            break;
        case 5:
            if(Punison_frequency_spread != value)
            {
                Punison_frequency_spread = value;
                calcFreqs();
            }
            break;
        case 6:
            if(value > 1)
                Pnegate = 1;
            else
                Pnegate = value;
            break;
        case 7:
            setlpf(value);
            break;
        case 8:
            sethpf(value);
            break;
        case 9:
        {
            auto lim = limit(value, (unsigned char) 1, (unsigned char) 3);
            if(Punison_size != lim)
            {
                Punison_size = lim;
                if (Punison_size>2) Ppreset=0;
                calcFreqs();
            }
            break;
        }
        case 10:
        {
            auto lim = limit(value, (unsigned char) 0, (unsigned char) 76);
            if(Pstrings != lim)
            {
                Pstrings = lim;
                if (Pstrings>6) Ppreset=0;
                calcFreqs();
            }
            break;
        }
        case 11:
            if (Pbasenote != value)
            {
                Pbasenote = value;
                baseFreq = powf(2.0f, ((float)Pbasenote-69.0f)/12.0f)*440.0f;
                calcFreqs();
            }
            break;
        case 12:
            PdropRate = value;
            filterBank->dropRate = (float)(PdropRate-64)/(-256.0f * float(srate));
            break;
        case 13:
            PmaxDrop = value;
            filterBank->maxDrop = (float)(PmaxDrop+1)/32.0f;
            break;
        case 14:
            PfadingTime = value;
            filterBank->fadingTime = (float)value/127.0f;
            break;
        case 15:
            PfreqOffset = value;
            filterBank->pitchOffset = (float)(PfreqOffset-64)/-64.0f;
            break;
        case 16: // Inharmonizität (I)
            if (Pinharmonicity!=value) {
                Pinharmonicity = value;
                // Skaliert 0..127 → 0.0 .. 1.0
                inharmonicity = (float)value / 127.0f;
                calcFreqs();
            }
            break;

        case 17: // Beta-Exponent
            if (Pbeta!=value) {
                Pbeta = value;
                // Skaliert 0..127 → 1.5 .. 0.5 (Saiten bis Metall)
                beta = 1.5f - ((float)value / 256.0f) * 2.0f;
                calcFreqs();
            }
            break;

        case 18: // Gamma-Exponent
            if (Pgamma!=value) {
                Pgamma = value;
                // Skaliert 0..127 → 1.0 .. 3.0 (harmonisch bis stark inharmonisch)
                gamma = 3.0f - ((float)value / 64.0f);
                calcFreqs();
            }
            break;

        case 19: // Contact Distance
            if (PcontactDist!=value) {
                PcontactDist = value;
                filterBank->contactOffset = (float)value / 127.0f;
            }
            break;

        case 20: // Contact Strength
            if (PcontactStrength!=value) {
                PcontactStrength = value;
                filterBank->contactStrength = (float)value / 127.0f;
            }
            break;

        default:
            break;
    }
}

unsigned char Sympathetic::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return Pq;
        case 3:  return Pdrive;
        case 4:  return Plevel;
        case 5:  return Punison_frequency_spread;
        case 6:  return Pnegate;
        case 7:  return Plpf;
        case 8:  return Phpf;
        case 9:  return Punison_size;
        case 10: return Pstrings;
        case 11: return Pbasenote;
        case 12: return PdropRate;
        case 13: return PmaxDrop;
        case 14: return PfadingTime;
        case 15: return PfreqOffset;
        case 16: return Pinharmonicity;
        case 17: return Pbeta;
        case 18: return Pgamma;
        case 19: return PcontactDist;
        case 20: return PcontactStrength;
        default: return 0; //in case of bogus parameter number
    }
}

}
