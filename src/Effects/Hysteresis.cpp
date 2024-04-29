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
            if(abs(i-192)<3 || abs(i-64)<3) printf("i: %d  x: %f  y: %f\n",i, orig[i], output[i]);
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
      drive(1.0f),
      remanence(0.5f),
      coercivity(0.5f)
{

    hyst_port = new Hyst;
    hyst_l = new Hyst;
    hyst_r = new Hyst;

}

Hysteresis::~Hysteresis()
{
    delete hyst_port;
    delete hyst_l;
    delete hyst_r;
}



inline float dualCos(float x, float drive, float par)
{
    float y;
    x *= drive;
    if (x > 1.0f)
        y = 1.0f;
    else if (x < -1.0f)
        y = -1.0f;
    else if (fabs(x)<par) 
            y = 0.0f;
        else {
            if (x>0)
            {
                const float smpTmp = (x-par)/(1.0f-par);
                y = 0.5 + 0.5 * cos(smpTmp*PI-PI);
            }
            else
            {
                const float smpTmp = (x+par)/(1.0f-par);
                y = -0.5 - 0.5 * cos(smpTmp*PI-PI);
            }
        }
    return y/drive;
}


// Include comments explaining the YehAbelSmith function
// Function calculates Yeh Abel Smith equation for given input x and exponent exp
inline float YehAbelSmith(float x, float exp)
{
    return x / (powf((1+powf(fabsf(x),exp)),(1/exp)));
}

void Hysteresis::calcHysteresis(float* input, float* output, int length, Hyst* hyst) {
    // Calculate parameter for YehAbelSmith function based on remanence
    float par = (6.0f) * remanence * remanence + (0.1f) * remanence + 0.25f;
    
    for(int i = 0; i < length; ++i) {
        const float x = input[i]*drive;
        const float xGradient = x - hyst->xLast;
        // Check for change of direction in input gradient
        if(xGradient * hyst->xLastGradient < 0.0f) { // change of direction
            
            // Determine offset and scaling factor based on input direction
            if (hyst->xLastGradient>0) {
                // Shift YehAbelSmith function to the right for positive input values
                const float xMax = (1.0f+coercivity)*drive;
                // Calculate offset for X and Y axes
                hyst->xOffset = xMax - x;
                const float yMax = YehAbelSmith(xMax, par);
                hyst->yFactor = 0.5f * (yMax + hyst->y);
                hyst->yOffset = 0.5f * ( yMax - hyst->y);
            }
            else {
                // Shift YehAbelSmith function to the left for negative input values
                const float xMax = (-1.0f-coercivity)*drive;
                // Calculate offset for X and Y axes
                hyst->xOffset = xMax - x;
                const float yMax = YehAbelSmith(xMax, par);
                hyst->yFactor = -0.5f * (yMax + hyst->y);
                hyst->yOffset = 0.5f * (yMax - hyst->y);
            }
        }
        
        // Calculate YehAbelSmith output for current input and apply scaling and offset
        hyst->y = (YehAbelSmith(x+hyst->xOffset, par) * hyst->yFactor - hyst->yOffset);
        
        // Calculate input gradient and update last input value
        hyst->xLastGradient = x - hyst->xLast;
        hyst->xLast = x;
        
        // Store calculated output in the output array
        output[i] = hyst->y;
    }
}


void Hysteresis::out(const Stereo<float *> &input)
{
    float output[buffersize];
    calcHysteresis(input.l, output, buffersize, hyst_l);
    for(int i = 0; i < buffersize; ++i) {
        efxoutl[i] = output[i];
        efxoutr[i] = output[i];
    }
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
    drive   = 0.1f + Pdrive/8.0f;
}

void Hysteresis::setremanence(unsigned char Premanence)
{
    remanence   = Premanence / 127.0f;

}

void Hysteresis::setcoercivity(unsigned char Pcoercivity)
{
    coercivity   = Pcoercivity / 127.0f;
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

    }
}

unsigned char Hysteresis::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return int((drive-0.1)*8.0f);
        case 3:  return int(remanence*127.0f);
        case 4:  return int(coercivity*127.0f);
        case 5:  return Pstereo;
        default: return 0; // in case of bogus parameter number
    }
}

}
