/*
  ZynAddSubFX - a software synthesizer

  ModMatrix.h - Modulation matrix implementation
  Copyright (C) Michael Kirchner
  Author: Michael Kirchner

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef MODMATRIX_H
#define MODMATRIX_H

#include "../globals.h"
#include "../Misc/Time.h"
#include "../Params/Presets.h"

namespace zyn {

#define MODMATRIX_SOURCES \
    MOD_PITCH,\
    MOD_VEL,\
    MOD_ENV1,\
    MOD_ENV2,\
    MOD_LFO1,\
    MOD_LFO2

#define NUM_MOD_MATRIX_SOURCES 6

enum {
    MODMATRIX_SOURCES
};

#define NUM_MOD_MATRIX_LOCATIONS 11
// Define the locations list for modmatrix. 
//ATTENTION: the order must be the same as the numbering in presets.h
#define MOD_LOCATIONS \
    AD_GLOBAL_AMP, AD_GLOBAL_FREQ, AD_GLOBAL_FILTER, \
    AD_VCE8_FM_AMP, AD_VCE8_FM_FREQ, \
    SUB_FREQ, SUB_FILTER, SUB_BANDWIDTH, \
    LOC_GENERIC1, LOC_GENERIC2, GLOBAL

enum {
    MOD_LOCATIONS
};

#define MOD_LOCATION_PARAMS \
DIRECT, \
LFO_FREQ, \
LFO_DEPTH, \
ENV_SPEED, \
ENV_DEPTH

#define NUM_MOD_MATRIX_PARAMETERS 5

enum {
    MOD_LOCATION_PARAMS
};

// place for location independent parameters
#define MOD_GLOBAL_PARAMS \
PAR_1, \
PAR_2, \
PAR_3, \
PAR_4, \
PAR_5, \
PAR_6, \
PAR_7, \
PAR_8, \
PAR_9

#define APPLY_MODMATRIX_FACTOR(PARS, paramVar, paramInd) \
    if (modValue != NULL) \
        for (auto i = 0; i < NUM_MOD_MATRIX_SOURCES; i++) \
        { const float f_##paramVar = PARS.mod->source[i]->getDestinationFactor(PARS.loc, paramInd); \
          if (modValue[i] && f_##paramVar) paramVar *= 1.0f + (modValue[i] * f_##paramVar); }

#define APPLY_MODMATRIX_DIRECT(MODMAT, LOC, paramVar) \
    for (auto i = 0; i < NUM_MOD_MATRIX_SOURCES; i++) \
        { const float f_##paramVar = MODMAT->source[i]->location[LOC]->getFactor(0); \
          if (NoteGlobalPar.modValue[i] && f_##paramVar) paramVar *= 1.0f + (NoteGlobalPar.modValue[i] * f_##paramVar); }

class ModulationLocation {
    
    public:
    ModulationLocation();
    float getFactor(int parameter);
    void setFactor(int parameter, float value);
    
    // public for ports:
    float parameter[NUM_MOD_MATRIX_PARAMETERS] = {};

    static const rtosc::Ports ports;

};

class ModulationSource {
    
    
    public:
    ModulationSource();
    ~ModulationSource();
    float getDestinationFactor(int location, int parameter);
    
    // public for ports:
    class  ModulationLocation* location[NUM_MOD_MATRIX_LOCATIONS];
    
    static const rtosc::Ports ports;
    
    private:
    int getIndex(int location, int parameter);
    int getLocationIndex(int location);
};


class ModMatrix
{
    public:
        /**Constructor
         *
         * @param lfopars pointer to a LFOParams object
         * @param basefreq base frequency of LFO
         */
        ModMatrix();
        ~ModMatrix();
        
        class ModulationSource* source[NUM_MOD_MATRIX_SOURCES];
        
        unsigned char PSources;
        unsigned char PDestLocations;
        unsigned char PDestParameters;
        
        static const rtosc::Ports ports;
};

}

#endif
