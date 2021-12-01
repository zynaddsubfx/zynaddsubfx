/*
  ZynAddSubFX - a software synthesizer

  SynthNote.cpp - Abstract Synthesizer Note Instance
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "SynthNote.h"
#include "../Params/Controller.h"
#include "../Misc/Util.h"
#include "../globals.h"
#include <cstring>
#include <new>
#include <iostream>

namespace zyn {

SynthNote::SynthNote(const SynthParams &pars)
    :memory(pars.memory),
    legato(pars.synth, pars.velocity, pars.portamento,
            pars.note_log2_freq, pars.quiet, pars.seed), ctl(pars.ctl), synth(pars.synth), time(pars.time)
{}

SynthNote::Legato::Legato(const SYNTH_T &synth_, float vel,
                          Portamento *portamento,
                          float note_log2_freq, bool quiet, prng_t seed)
    :synth(synth_)
{
    // Initialise some legato-specific vars
    msg = LM_Norm;
    fade.length = (int)(synth.samplerate_f * 0.005f);      // 0.005f seems ok.
    if(fade.length < 1)
        fade.length = 1;                    // (if something's fishy)
    fade.step  = (1.0f / fade.length);
    decounter  = -10;
    param.vel  = vel;
    param.portamento = portamento;
    param.note_log2_freq = note_log2_freq;
    param.seed = seed;
    lastfreq_log2 = note_log2_freq;
    silent   = quiet;
}

int SynthNote::Legato::update(const LegatoParams &pars)
{
    if(pars.externcall)
        msg = LM_Norm;
    if(msg != LM_CatchUp) {
        lastfreq_log2 = param.note_log2_freq;
        param.vel  = pars.velocity;
        param.portamento = pars.portamento;
        param.note_log2_freq = pars.note_log2_freq;
        if(msg == LM_Norm) {
            if(silent) {
                fade.m = 0.0f;
                msg    = LM_FadeIn;
            }
            else {
                fade.m = 1.0f;
                msg    = LM_FadeOut;
                return 1;
            }
        }
        if(msg == LM_ToNorm)
            msg = LM_Norm;
    }
    return 0;
}

void SynthNote::Legato::apply(SynthNote &note, float *outl, float *outr)
{
    if(silent) // Silencer
        if(msg != LM_FadeIn) {
            memset(outl, 0, synth.bufferbytes);
            memset(outr, 0, synth.bufferbytes);
        }
    try {
        switch (msg) {
            case LM_CatchUp: // Continue the catch-up...
                if (decounter == -10)
                    decounter = fade.length;
                //Yea, could be done without the loop...
                for (int i = 0; i < synth.buffersize; ++i) {
                    decounter--;
                    if (decounter < 1) {
                        // Catching-up done, we can finally set
                        // the note to the actual parameters.
                        decounter = -10;
                        msg = LM_ToNorm;
                        LegatoParams pars{param.vel, param.portamento,
                                          param.note_log2_freq, false, param.seed};
                        note.legatonote(pars);
                        break;
                    }
                }
                break;
            case LM_FadeIn: // Fade-in
                if (decounter == -10)
                    decounter = fade.length;
                silent = false;
                for (int i = 0; i < synth.buffersize; ++i) {
                    decounter--;
                    if (decounter < 1) {
                        decounter = -10;
                        msg = LM_Norm;
                        break;
                    }
                    fade.m += fade.step;
                    outl[i] *= fade.m;
                    outr[i] *= fade.m;
                }
                break;
            case LM_FadeOut: // Fade-out, then set the catch-up
                if (decounter == -10)
                    decounter = fade.length;
                for (int i = 0; i < synth.buffersize; ++i) {
                    decounter--;
                    if (decounter < 1) {
                        for (int j = i; j < synth.buffersize; ++j) {
                            outl[j] = 0.0f;
                            outr[j] = 0.0f;
                        }
                        decounter = -10;
                        silent = true;
                        // Fading-out done, now set the catch-up :
                        decounter = fade.length;
                        msg = LM_CatchUp;
                        //This freq should make this now silent note to catch-up/resync
                        //with the heard note for the same length it stayed at the
                        //previous freq during the fadeout.
                        const float catchupfreq_log2 = 2.0f * param.note_log2_freq - lastfreq_log2;
                        LegatoParams pars{param.vel, param.portamento,
                            catchupfreq_log2, false, param.seed};
                        note.legatonote(pars);
                        break;
                    }
                    fade.m -= fade.step;
                    outl[i] *= fade.m;
                    outr[i] *= fade.m;
                }
                break;
            default:
                break;
        }
    } catch (std::bad_alloc &ba) {
        std::cerr << "failed to apply legato: " << ba.what() << std::endl;
    }
}

void SynthNote::setVelocity(float velocity_) {
    legato.setSilent(true); //Let legato.update(...) returns 0.
    LegatoParams pars{velocity_,
               legato.getPortamento(), legato.getNoteLog2Freq(), true, legato.getSeed()};
    try {
        legatonote(pars);
    } catch (std::bad_alloc &ba) {
        std::cerr << "failed to set velocity to legato note: " << ba.what() << std::endl;
    }
    legato.setDecounter(0); //avoid chopping sound due fade-in
}

void SynthNote::setPitch(float log2_freq_) {
    legato.setSilent(true); //Let legato.update(...) return 0.
    LegatoParams pars{legato.getVelocity(),
               legato.getPortamento(), log2_freq_, true, legato.getSeed()};
    try {
        legatonote(pars);
    } catch (std::bad_alloc &ba) {
        std::cerr << "failed to set velocity to legato note: " << ba.what() << std::endl;
    }
    legato.setDecounter(0); //avoid chopping sound due fade-in
}

void SynthNote::setFilterCutoff(float value)
{
    /* set current value, if first time */
    if (filtercutoff_relfreq.isSet() == false)
        filtercutoff_relfreq = ctl.filtercutoff.relfreq;
    /* update value */
    filtercutoff_relfreq =
        (value - 64.0f) * ctl.filtercutoff.depth / 4096.0f * 3.321928f;
}

float SynthNote::getFilterCutoffRelFreq(void)
{
    if (filtercutoff_relfreq.isSet() == false)
        return (ctl.filtercutoff.relfreq);
    else
        return (filtercutoff_relfreq);
}

float SynthNote::getRandomFloat() {
    return (getRandomUint() / (INT32_MAX_FLOAT * 1.0f));
}

prng_t SynthNote::getRandomUint() {
    current_prng_state = prng_r(current_prng_state);
    return current_prng_state;
}

}
