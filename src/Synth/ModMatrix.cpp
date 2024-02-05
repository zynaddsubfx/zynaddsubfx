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

#define rObject ModulationLocation
#define rBegin [](const char *msg, RtData &d) { rObject &o = *(rObject*)d.obj
#define rEnd }
        
const Ports ModulationLocation::ports = {
    rSelf(ModulationLocation::location),
    rArrayF(parameter, NUM_MOD_MATRIX_PARAMETERS, rLinear(0.0f,100.0f), rDefault(0.0f), rUnit(%), "Modulation Matrix Factor"),
};

#undef  rObject
#define rObject ModulationSource
        
const Ports ModulationSource::ports = {
    rSelf(ModulationSource::source),
    rRecursp(location, NUM_MOD_MATRIX_LOCATIONS, "Location"),
};

#undef  rObject
#define rObject ModMatrix

const rtosc::Ports ModMatrix::ports = {
    rSelf(Matrix),
    rRecursp(source, NUM_MOD_MATRIX_SOURCES, "Source"),
    rOption(PSources,   rShort("source"),
          rOptions(MODMATRIX_SOURCES),
          "Modulation Matrix Source"),
    rOption(PDestLocations,   rShort("loc"),
          rOptions(MOD_LOCATIONS),
          "Modulation Matrix Destination Locations"),
    rOption(PDestParameters,   rShort("par"),
          rOptions(MOD_LOCATION_PARAMS),
          "Modulation Matrix Destination Parameters"),
};
#undef rBegin
#undef rEnd    
    
    

ModMatrix::ModMatrix()
{
    
    for(int i = 0; i < NUM_MOD_MATRIX_SOURCES; i++)
    {
        source[i] = new ModulationSource();
    }
}

ModulationSource::ModulationSource()
{
    for(auto i = 0; i < NUM_MOD_MATRIX_LOCATIONS; i++)
    {
        location [i] = new ModulationLocation();
    }
}

ModulationSource::~ModulationSource()
{
    for(auto i = 0; i < NUM_MOD_MATRIX_LOCATIONS; i++)
    {
        delete location[i];
    }

    delete[] location;
}

ModulationLocation::ModulationLocation()
{
    for(auto i = 0; i < NUM_MOD_MATRIX_PARAMETERS; i++)
    {
        parameter [i] = 0.0f;
    }
}


int ModulationSource::getLocationIndex(int location)
{
    int index;
    if (location < 3) index = location;
    else if (location < 6 ) index = -1;
    else if (location < 12 ) index = location-3;
    else if (location < 13 ) index = -1;
    else if (location < NUM_MOD_MATRIX_LOCATIONS) index = location-4;
    return index;
    
}

float ModulationLocation::getFactor(int par)
{
    return parameter[par] * 0.01f;
}

void ModulationLocation::setFactor(int par, float value)
{
    parameter[par] = value;
}

float ModulationSource::getDestinationFactor(int loc, int par)
{
    return location[loc]->getFactor(par);
}

void ModulationSource::setDestinationFactor(int loc, int par, float value)
{
    location[loc]->setFactor(par, value);
}

ModMatrix::~ModMatrix()
{

}

}
