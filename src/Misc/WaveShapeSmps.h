/*
  ZynAddSubFX - a software synthesizer

  WaveShapeSmps.h - Sample distortions
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#ifndef WAVESHAPESMPS_H
#define WAVESHAPESMPS_H

namespace zyn {

// Originalversion MIT den neuen Parametern
void waveShapeSmps(int buffersize,
                   float *smps,
                   unsigned char type,
                   unsigned char drive,
                   unsigned char offset,
                   unsigned char funcpar,
                   bool loudnessCompEnabled,
                   unsigned int &windowPos,
                   float &sumIn, float &sumOut,
                   float &aWeightHistIn, float &aWeightHistOut,
                   float &compensationfactor);

// Überladene Version OHNE die neuen Parameter
void waveShapeSmps(int buffersize,
                   float *smps,
                   unsigned char type,
                   unsigned char drive,
                   unsigned char offset = 64,
                   unsigned char funcpar = 0,
                   bool loudnessCompEnabled = false);

//calculate the polyblamp residual value (called by waveshape function)
float polyblampres(float smp,
                   float ws,
                   float dMax);


}

#endif
