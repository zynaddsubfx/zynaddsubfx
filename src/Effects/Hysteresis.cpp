/*
  ZynAddSubFX - a software synthesizer

  Hysteresis.cpp - Hysteresis effect
  Copyright (C) 2024 Michael Kirchner
  Author: Michael Kirchner

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cmath>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include "../Misc/Allocator.h"
#include "../DSP/AnalogFilter.h"
#include "Hysteresis.h"


namespace zyn {

#define rObject Hysteresis
#define rBegin [](const char *msg, rtosc::RtData &d) {
#define rEnd }

rtosc::Ports Hysteresis::ports = {
    {"preset::i", rOptions(Hysteresis 1, Hysteresis 2)
                  rProp(parameter)
                  rDoc("Instrument Presets"), 0,
                  rBegin;
                  rObject *o = (rObject*)d.obj;
                  if(rtosc_narguments(msg))
                      o->setpreset(rtosc_argument(msg, 0).i);
                  else
                      d.reply(d.loc, "i", o->Ppreset);
                  rEnd},
    rEffParVol(rDefault(64)),
    rEffParPan(),
    rEffPar(Pdrive,   2, rShort("drive"),
            "Drive of Hysteresis"),
    rEffPar(Premanence,   3, rShort("rem"),
            "Remanence of Hysteresis"),
    rEffPar(Pcoercivity,  4, rShort("coerc"),
            "Coercivity of Hysteresis"),
    rEffParTF(Pstereo, 5, rShort("stereo"),
          , "Stereo"),
    rEffPar(Plevel,   6, rShort("output"),
            "Output amplification"),
    rEffPar(Plpf, 7, rShort("lpf"), rDefault(127), "Low Pass Cutoff"),
    rEffPar(Phpf, 8, rShort("hpf"), rDefault(0), "High Pass Cutoff"),
    {"waveform:", 0, 0, [](const char *, rtosc::RtData &d)
    {
        Hysteresis  &dd = *(Hysteresis*)d.obj;
        float        orig[320], output[320];
        rtosc_arg_t  args[320];
        char         arg_str[320+1] = {};

        for(int i=0; i<320; i++) {
        if (i < 64)
            orig[i] = (float(i)/64.0f);
        else if(i < 192)
            orig[i] = 1.0f - (float(i-64)/64.0f);
        else
            orig[i] = (float(i-256)/64.0f);
       }

        dd.calcHysteresis(orig, output, 320, dd.hyst_port);

        for(int i=0; i<320; i++) {
            arg_str[i] = 'f';
            args[i].f  = (dd.Pvolume * output[i] + (127.0f - dd.Pvolume) * orig[i]) / 127.0f;
            //~ if(abs(i-192)<3 || abs(i-64)<3) printf("i: %d  x: %f  y: %f\n",i, orig[i], output[i]);
            //~ if(abs(i-64)<3) printf("i: %d  x: %f  y: %f\n",i, orig[i], output[i]);
        }

        d.replyArray(d.loc, arg_str, args);
        
    }},

};
#undef rBegin
#undef rEnd
#undef rObject

Hysteresis::Hysteresis(EffectParams pars)
    :Effect(pars),
      Pvolume(64),
      Pstereo(1),
      Plpf(127),
      Phpf(0),
      drive(1.0f),
      remanence(0.5f),
      coercivity(0.5f),
      level(16.0f)
{

    hyst_port = memory.alloc<Hyst>();
    hyst_l = memory.alloc<Hyst>();
    hyst_r = memory.alloc<Hyst>();
    
    lpfl = memory.alloc<AnalogFilter>(2, 22000, 1, 0, pars.srate, pars.bufsize);
    lpfr = memory.alloc<AnalogFilter>(2, 22000, 1, 0, pars.srate, pars.bufsize);
    hpfl = memory.alloc<AnalogFilter>(3, 20, 1, 0, pars.srate, pars.bufsize);
    hpfr = memory.alloc<AnalogFilter>(3, 20, 1, 0, pars.srate, pars.bufsize);

}

Hysteresis::~Hysteresis()
{
    memory.dealloc(hyst_port);
    memory.dealloc(hyst_l);
    memory.dealloc(hyst_r);
    
    memory.dealloc(lpfl);
    memory.dealloc(lpfr);
    memory.dealloc(hpfl);
    memory.dealloc(hpfr);
}

//Cleanup the effect
void Hysteresis::cleanup(void)
{
    lpfl->cleanup();
    hpfl->cleanup();
    lpfr->cleanup();
    hpfr->cleanup();
    
    hyst_port->xLast = 0.0f;
    hyst_l->xLast = 0.0f;
    hyst_r->xLast = 0.0f;
    
    hyst_port->y = 0.0f;
    hyst_l->y = 0.0f;
    hyst_r->y = 0.0f;
    
    
}

//Apply the filters
void Hysteresis::applyfilters(float *efxoutl, float *efxoutr)
{
    if(Plpf!=127) lpfl->filterout(efxoutl);
    hpfl->filterout(efxoutl);
    if(Pstereo != 0) { //stereo
        if(Plpf!=127) lpfr->filterout(efxoutr);
        hpfr->filterout(efxoutr);
    }
}
// Function calculates Yeh Abel Smith equation for given input x and exponent exp
inline float YehAbelSmith(float x, float exp)
{
    return x / (powf((1+powf(fabsf(x),exp)),(1/exp)));
}

inline void Hysteresis::updatePar()
{
    // Shift YehAbelSmith function to the right end if dx changed from pos to neg
    xRight = (1.0f+coercivity)*drive;
    // get the corresponing y value
    yRight = YehAbelSmith(xRight, par);
    
    // Shift YehAbelSmith function to the left end if dx changed from neg to pos
    xLeft = (-1.0f-coercivity)*drive;
    // get the corresponing y value
    yLeft = YehAbelSmith(xLeft, par);
}

void Hysteresis::calcHysteresis(float* input, float* output, int length, Hyst* hyst) {

    for(int i = 0; i < length; ++i) {
        const float x = input[i]*drive;
        const float xGradient = x - hyst->xLast;
        // Check for change of direction in input gradient
        if(xGradient * hyst->dxLast < 0.0f) { 
            // Determine offset and scaling factor based on input direction
            if (hyst->dxLast>0) {
                
                // Calculate offset for current x to xMax
                hyst->xOffset = xRight - x;

                // Calculate y factor and offset to compensate the change due to offsetting x
                // set factor to hit the y value between current and yMax
                hyst->yFactor = 0.5f * (yRight + hyst->y);
                // set the y offset to bridge the gap above and keep the other end of the curve at -yMax
                hyst->yOffset = (yRight*hyst->yFactor - hyst->y);
                
            }
            else {
                
                // Calculate offset for current x to xMax
                hyst->xOffset = xLeft - x;
                
                // Calculate y factor and offset to compensate the change due to offsetting x
                // set factor to hit the y value between current and yMax
                hyst->yFactor = -0.5f * (yLeft + hyst->y);
                // set the y offset to bridge the gap above and keep the other end of the curve at yMax
                hyst->yOffset = (yLeft*hyst->yFactor - hyst->y);
            }
        }
        
        // Calculate YehAbelSmith output for current input and apply scaling and offset
        hyst->y = (YehAbelSmith((x + hyst->xOffset), par)) * hyst->yFactor - hyst->yOffset;
        
        // Calculate input gradient and update last input value
        hyst->dxLast = x - hyst->xLast;
        hyst->xLast = x;
        
        // Store calculated output in the output array
        output[i] = hyst->y;
    }
}


void Hysteresis::out(const Stereo<float *> &input)
{
    float input_l[buffersize];
    float input_r[buffersize];
    float output_l[buffersize];
    float output_r[buffersize];

    if(Pstereo) { //Stereo
        for(int i = 0; i < buffersize; ++i) {
            input_l[i] = input.l[i] * pangainL;
            input_r[i] = input.r[i] * pangainR;
        }
    }
    else {//Mono
        for(int i = 0; i < buffersize; ++i)
            efxoutl[i] = (input.l[i] * pangainL + input.r[i] * pangainR);
    }
    
    calcHysteresis(input_l, output_l, buffersize, hyst_l);
    calcHysteresis(input_r, output_r, buffersize, hyst_r);
    

    for(int i = 0; i < buffersize; ++i) 
        efxoutl[i] = output_l[i]*level;
    
    if(Pstereo)
        for(int i = 0; i < buffersize; ++i) 
            efxoutr[i] = output_r[i]*level;
    else
        for(int i = 0; i < buffersize; ++i) 
            efxoutr[i] = output_l[i]*level;
        
    applyfilters(efxoutl, efxoutr);
    
}


//Parameter control
void Hysteresis::setvolume(unsigned char _Pvolume)
{
    Pvolume = _Pvolume;

    if(insertion == 0) {
        if (Pvolume == 0) {
            outvolume = 0.0f;
        } else {
            outvolume = powf(0.01f, (1.0f - Pvolume / 127.0f)) * 4.0f;
        }
        volume    = 1.0f;
    }
    else
        volume = outvolume = Pvolume / 127.0f;
}

void Hysteresis::setdrive(unsigned char Pdrive)
{
    drive   = 0.5f + Pdrive/8.0f;
    updatePar();
}

void Hysteresis::setremanence(unsigned char Premanence)
{
    remanence   = Premanence / 127.0f;
    // Calculate parameter for YehAbelSmith function based on remanence
    par = (6.0f) * remanence * remanence + (0.1f) * remanence + 0.25f;
    updatePar();

}

void Hysteresis::setcoercivity(unsigned char Pcoercivity)
{
    coercivity   = Pcoercivity / 127.0f;
    updatePar();
}

void Hysteresis::setlpf(unsigned char _Plpf)
{
    Plpf = _Plpf;
    float fr = expf(sqrtf(Plpf / 127.0f) * logf(25000.0f)) + 40.0f;
    lpfl->setfreq(fr);
    lpfr->setfreq(fr);
}

void Hysteresis::sethpf(unsigned char _Phpf)
{
    Phpf = _Phpf;
    float fr = expf(sqrtf((Phpf+1) / 128.0f) * logf(25000.0f)) + 20.0f;
    hpfl->setfreq(fr);
    hpfr->setfreq(fr);
}

unsigned char Hysteresis::getpresetpar(unsigned char npreset, unsigned int npar)
{
#define	PRESET_SIZE 3
#define	NUM_PRESETS 1
    static const unsigned char presets[NUM_PRESETS][PRESET_SIZE] = {
        {67, 64, 35 }, //Hysteresis 1

    };
    if(npreset < NUM_PRESETS && npar < PRESET_SIZE) {
        if(npar == 0 && insertion != 0) {
            /* lower the volume if this is insertion effect */
            return presets[npreset][npar] / 2;
        }
        return presets[npreset][npar];
    }
    return 0;
}

void Hysteresis::setpreset(unsigned char npreset)
{
    if(npreset >= NUM_PRESETS)
        npreset = NUM_PRESETS - 1;
    for(int n = 0; n != 128; n++)
        changepar(n, getpresetpar(npreset, n));
    Ppreset = npreset;
}

void Hysteresis::changepar(int npar, unsigned char value)
{
    switch(npar) {
        case 0:
            setvolume(value);
            break;
        case 1:
            setpanning(value);
            break;
        case 2:
            setdrive(value);
            break;
        case 3:
            setremanence(value);
            break;
        case 4:
            setcoercivity(value);
            break;
        case 5:
            Pstereo = (value > 1) ? 1 : value;
            break;
        case 6:
            level = float(value)/4.0f;
            break;
        case 7:
            setlpf(value);
            break;
        case 8:
            sethpf(value);
            break;

    }
}

unsigned char Hysteresis::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return int((drive-0.5)*8.0f);
        case 3:  return int(remanence*127.0f);
        case 4:  return int(coercivity*127.0f);
        case 5:  return Pstereo;
        case 6:  return int(level*4.0f);
        case 7:  return Plpf;
        case 8:  return Phpf;
        default: return 0; // in case of bogus parameter number
    }
}

}
