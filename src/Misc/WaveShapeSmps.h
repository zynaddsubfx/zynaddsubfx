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
                   const unsigned char type,
                   const unsigned char drive,
                   const unsigned char driveHist,
                   const unsigned char offset,
                   const unsigned char funcpar,
                   const unsigned char loudnessComp,
                   float &sumIn, float &sumOut,
                   float &aWeightHistIn, float &aWeightHistOut,
                   float &compensationfactor);

// Überladene Version OHNE die neuen Parameter
void waveShapeSmps(int buffersize,
                   float *smps,
                   const unsigned char type,
                   const unsigned char drive,
                   const unsigned char offset = 64,
                   const unsigned char funcpar = 0);

//calculate the polyblamp residual value (called by waveshape function)
float polyblampres(const float smp,
                   const float ws,
                   const float dMax);


}

#endif
