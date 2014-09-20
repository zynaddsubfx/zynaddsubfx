#include "SynthNote.h"
#include "../globals.h"
#include "../Params/Controller.h"
#include "../Misc/Util.h"
#include <cmath>
#include <cstring>

                
void PunchState::init(char Strength, char VelocitySensing,
        char Time, char Stretch, float velocity, float freq)
{
        if(Strength) {
            Enabled = 1;
            t = 1.0f; //start from 1.0f and to 0.0f
            initialvalue = (powf(10, 1.5f * Strength / 127.0f) - 1.0f)
                 * VelF(velocity, VelocitySensing);
            //0.1 to 100 ms 
            float time = powf(10, 3.0f * Time / 127.0f) / 10000.0f;
            float stretch = powf(440.0f / freq, Stretch / 64.0f);
            dt = 1.0f / (time * synth->samplerate_f * stretch);
        }
        else
            Enabled = 0;
}
SynthNote::SynthNote(SynthParams &pars)
    :legato(pars.frequency, pars.velocity, pars.portamento,
            pars.note, pars.quiet),
     memory(pars.memory),
     ctl(pars.ctl)
{}

SynthNote::Legato::Legato(float freq, float vel, int port,
                          int note, bool quiet)
{
    // Initialise some legato-specific vars
    msg = LM_Norm;
    fade.length = (int)(synth->samplerate_f * 0.005f);      // 0.005f seems ok.
    if(fade.length < 1)
        fade.length = 1;                    // (if something's fishy)
    fade.step  = (1.0f / fade.length);
    decounter  = -10;
    param.freq = freq;
    param.vel  = vel;
    param.portamento = port;
    param.midinote   = note;
    lastfreq = 0.0f;
    silent   = quiet;
}

int SynthNote::Legato::update(LegatoParams pars)
{
    if(pars.externcall)
        msg = LM_Norm;
    if(msg != LM_CatchUp) {
        lastfreq   = param.freq;
        param.freq = pars.frequency;
        param.vel  = pars.velocity;
        param.portamento = pars.portamento;
        param.midinote   = pars.midinote;
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
            memset(outl, 0, synth->bufferbytes);
            memset(outr, 0, synth->bufferbytes);
        }
    switch(msg) {
        case LM_CatchUp: // Continue the catch-up...
            if(decounter == -10)
                decounter = fade.length;
            //Yea, could be done without the loop...
            for(int i = 0; i < synth->buffersize; ++i) {
                decounter--;
                if(decounter < 1) {
                    // Catching-up done, we can finally set
                    // the note to the actual parameters.
                    decounter = -10;
                    msg = LM_ToNorm;
                    LegatoParams pars{param.freq, param.vel, param.portamento,
                                    param.midinote, false};
                    note.legatonote(pars);
                    break;
                }
            }
            break;
        case LM_FadeIn: // Fade-in
            if(decounter == -10)
                decounter = fade.length;
            silent = false;
            for(int i = 0; i < synth->buffersize; ++i) {
                decounter--;
                if(decounter < 1) {
                    decounter = -10;
                    msg = LM_Norm;
                    break;
                }
                fade.m  += fade.step;
                outl[i] *= fade.m;
                outr[i] *= fade.m;
            }
            break;
        case LM_FadeOut: // Fade-out, then set the catch-up
            if(decounter == -10)
                decounter = fade.length;
            for(int i = 0; i < synth->buffersize; ++i) {
                decounter--;
                if(decounter < 1) {
                    for(int j = i; j < synth->buffersize; ++j) {
                        outl[j] = 0.0f;
                        outr[j] = 0.0f;
                    }
                    decounter = -10;
                    silent    = true;
                    // Fading-out done, now set the catch-up :
                    decounter = fade.length;
                    msg = LM_CatchUp;
                    //This freq should make this now silent note to catch-up/resync
                    //with the heard note for the same length it stayed at the
                    //previous freq during the fadeout.
                    float catchupfreq = param.freq * (param.freq / lastfreq);
                    LegatoParams pars{catchupfreq, param.vel, param.portamento,
                        param.midinote, false};
                    note.legatonote(pars);
                    break;
                }
                fade.m  -= fade.step;
                outl[i] *= fade.m;
                outr[i] *= fade.m;
            }
            break;
        default:
            break;
    }
}

void SynthNote::setVelocity(float velocity_) {
    legato.setSilent(true); //Let legato.update(...) returns 0.
    LegatoParams pars{legato.getFreq(), velocity_,
               legato.getPortamento(), legato.getMidinote(), true};
    legatonote(pars);
    legato.setDecounter(0); //avoid chopping sound due fade-in
}

void SynthNote::fadeinCheap(float *smpl, float *smpr) const
{
    int n = 10;
    if(n > synth->buffersize)
        n = synth->buffersize;
    for(int i = 0; i < n; ++i) {
        float ampfadein = 0.5f - 0.5f * cosf(i / (PI * n));
        smpl[i] *= ampfadein;
        smpr[i] *= ampfadein;
    }
}

void SynthNote::fadein(float *smps) const
{
    int zerocrossings = 0;
    for(int i = 1; i < synth->buffersize; ++i)
        if(smps[i - 1] < 0.0f && smps[i] > 0.0f)
            zerocrossings++;  //this is only the possitive crossings

    float tmp = (synth->buffersize_f - 1.0f) / (zerocrossings + 1) / 3.0f;
    if(tmp < 8.0f)
        tmp = 8.0f;

    int n;
    F2I(tmp, n); //how many samples is the fade-in
    if(n > synth->buffersize)
        n = synth->buffersize;
    for(int i = 0; i < n; ++i) { //fade-in
        float tmp = 0.5f - cosf((float)i / (float) n * PI) * 0.5f;
        smps[i] *= tmp;
    }
}

void SynthNote::fadeout(float *smps) const
{
    for(int i = 0; i < synth->buffersize; ++i)
        smps[i] *= 1.0f - (float)i / synth->buffersize_f;
}

void SynthNote::applyPunch(float *outl, float *outr, PunchState &Punch)
{
    if(Punch.Enabled)
        for(int i = 0; i < synth->buffersize; ++i) {
            float punchamp = Punch.initialvalue
                             * Punch.t + 1.0f;
            outl[i] *= punchamp;
            outr[i] *= punchamp;
            Punch.t -= Punch.dt;
            if(Punch.t < 0.0f) {
                Punch.Enabled = false;
                break;
            }
        }
}

void SynthNote::applyPanning(float *outl, float *outr, float oldamp, float newamp,
                             float panl, float panr)
{
    if(ABOVE_AMPLITUDE_THRESHOLD(oldamp, newamp))
        // Amplitude Interpolation
        for(int i = 0; i < synth->buffersize; ++i) {
            float tmpvol = INTERPOLATE_AMPLITUDE(oldamp,
                                                 newamp,
                                                 i,
                                                 synth->buffersize);
            outl[i] *= tmpvol * panl;
            outr[i] *= tmpvol * panr;
        }
    else
        for(int i = 0; i < synth->buffersize; ++i) {
            outl[i] *= newamp * panl;
            outr[i] *= newamp * panr;
        }

}
        
void SynthNote::applyAmp(float *outl, float *outr, float oldamp, float newamp)
{
    //TODO see if removing this "rest" bit of the algorithm does anything
    //noteworthy
    //it would look like it does not, however all sorts of odd things might make
    //a difference
    if(ABOVE_AMPLITUDE_THRESHOLD(oldamp, newamp)) {
        int rest = synth->buffersize;
        //test if the amplitude if raising and the difference is high
        if((newamp > oldamp) && ((newamp - oldamp) > 0.25f)) {
            rest = 10;
            if(rest > synth->buffersize)
                rest = synth->buffersize;
            for(int i = 0; i < synth->buffersize - rest; ++i)
                outl[i] *= oldamp;
            if(outr)
                for(int i = 0; i < synth->buffersize - rest; ++i)
                    outr[i] *= oldamp;
        }
        // Amplitude interpolation
        for(int i = 0; i < rest; ++i) {
            float amp = INTERPOLATE_AMPLITUDE(oldamp, newamp, i, rest);
            outl[i + (synth->buffersize - rest)] *= amp;
            if(outr)
                outr[i + (synth->buffersize - rest)] *= amp;
        }
    }
    else {
        for(int i = 0; i < synth->buffersize; ++i)
            outl[i] *= newamp;
        if(outr)
            for(int i = 0; i < synth->buffersize; ++i)
                outr[i] *= newamp;
    }
}

float SynthNote::applyFixedFreqET(bool fixedfreq_, float basefreq, int fixedfreqET,
         float detune, int midinote) const
{
    if(fixedfreq_) {
        float fixedfreq   = 440.0f;
        if(fixedfreqET != 0) { //if the frequency varies according the keyboard note
            float tmp = (midinote - 69.0f) / 12.0f
                * (powf(2.0f, (fixedfreqET - 1) / 63.0f) - 1.0f);
            if(fixedfreqET <= 64)
                fixedfreq *= powf(2.0f, tmp);
            else
                fixedfreq *= powf(3.0f, tmp);
        }
        return fixedfreq * powf(2.0f, detune / 12.0f);
    } else
        return basefreq * powf(2, detune / 12.0f);
}

float SynthNote::getPortamento(bool &portamento) const
{
    if(portamento) { //this voice use portamento
        if(!ctl.portamento.used) //the portamento has finished
            portamento = false;  //this note is no longer "portamented"
        return ctl.portamento.freqrap;
    }
    return 1.0;
}

