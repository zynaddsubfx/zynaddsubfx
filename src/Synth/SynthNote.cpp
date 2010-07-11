#include "SynthNote.h"
#include "../globals.h"

SynthNote::SynthNote(REALTYPE freq, REALTYPE vel, int port, int note, bool quiet) 
    :legato(freq,vel,port,note,quiet)
{}

SynthNote::Legato::Legato(REALTYPE freq, REALTYPE vel, int port, 
                          int note, bool quiet)
{
    // Initialise some legato-specific vars
    msg = LM_Norm;
    fade.length      = (int)(SAMPLE_RATE * 0.005); // 0.005 seems ok.
    if(fade.length < 1)
        fade.length = 1;                    // (if something's fishy)
    fade.step        = (1.0 / fade.length);
    decounter        = -10;
    param.freq       = freq;
    param.vel        = vel;
    param.portamento = port;
    param.midinote   = note;
    silent  = quiet;
}
