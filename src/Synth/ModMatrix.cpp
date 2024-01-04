/*
  ZynAddSubFX - a software synthesizer

  ModMatrix.cpp - Modulation matrix implementation
  Copyright (C) Michael Kirchner
  Author: Michael Kirchner

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/


#include "ModMatrix.h"
#include <cstring>
#include "../Misc/Util.h"
#include "../Params/Presets.h"
#include <rtosc/port-sugar.h>
#include <rtosc/ports.h>

namespace zyn {
    
using rtosc::Ports;
using rtosc::RtData;

#define rObject ModulationSource
#define rBegin [](const char *msg, RtData &d) { rObject &o = *(rObject*)d.obj
#define rEnd }
const Ports ModulationSource::ports = {
    rSelf(ADnoteGlobalParam::source, rEnabledBy(Penabled)),
    rArrayF(destination, NUM_MOD_MATRIX_DESTINATIONS, "Modulation Matrix Factor"),
};

#undef  rObject
#define rObject ModMatrix

const rtosc::Ports ModMatrix::ports = {
    rSelf(Matrix),
    rRecursp(source, NUM_MOD_MATRIX_SOURCES, "Source"),
    rOption(PSources,   rShort("source"),
          rOptions(MODMATRIX_SOURCES),
          "Modulation Matrix Source"),
    rOption(PDestinations,   rShort("dest"),
          rOptions(MODMATRIX_DESTINATIONS),
          "Modulation Matrix Source"),
};
#undef rBegin
#undef rEnd    
    
    

ModMatrix::ModMatrix()
{
    value = new float[NUM_MOD_MATRIX_SOURCES];    
    
    for(int nsource = 0; nsource < NUM_MOD_MATRIX_SOURCES; ++nsource)
    {
        source[nsource] = new ModulationSource();
    }
}

float ModulationSource::getDestinationFactor(int location, int parameter)
{
    int index; 
    if (location<NUM_LOCATIONS)
        index = NUM_MODMATRIX_ANY_DESTINATIONS + (location * NUM_MODMATRIX_LFO_DESTINATIONS) + parameter;
    else if (parameter < NUM_MODMATRIX_ANY_DESTINATIONS)
        index = parameter;
    else
        index = 0;
        
    return destination[index];
}

ModMatrix::~ModMatrix()
{
    delete[] value;

}

}
