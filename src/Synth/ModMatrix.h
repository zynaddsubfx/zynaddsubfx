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
    GEN_ENV1,\
    GEN_ENV2,\
    GEN_LFO1,\
    GEN_LFO2

#define NUM_MOD_MATRIX_SOURCES 6

enum {
    MODMATRIX_SOURCES
};

#define NUM_MOD_LOCATIONS 15
// Define the locations list for modmatrix. 
//ATTENTION: the order must be the same as the numbering in presets.h
#define MOD_LOCATIONS \
    AD_GLOBAL_AMP, AD_GLOBAL_FREQ, AD_GLOBAL_FILTER, \
    AD_VOICE_AMP, AD_VOICE_FREQ, AD_VOICE_FILTER, AD_VOICE_FM_AMP, AD_VOICE_FM_FREQ, \
    SUB_FREQ, SUB_FILTER, SUB_BANDWIDTH, \
    IN_EFFECT, LOC_GENERIC1, LOC_GENERIC2, GLOBAL

#define MOD_LOCATION_PARAMS \
DIRECT, \
PAR_LFO_FREQ, \
PAR_LFO_DEPTH, \
PAR_ENV_A, \
PAR_ENV_D, \
PAR_ENV_S, \
PAR_ENV_R, \
PAR_ENV_SPEED, \
PAR_ENV_DEPTH

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

#define NUM_LOCATION_PARAMS 9

#define NUM_MOD_MATRIX_DESTINATIONS NUM_MOD_LOCATIONS*NUM_LOCATION_PARAMS


#define APPLY_MODMATRIX_FACTOR_IMPL(param) \
    const float f_##param = lfopars.mod->source[i]->getDestinationFactor(lfopars.loc, param); \
    if (modValue[i] && f_##param) param *= modValue[i] * f_##param;

#define APPLY_MODMATRIX_FACTOR(p1, p2, param) APPLY_MODMATRIX_FACTOR_IMPL(param)

#define APPLY_MODMATRIX_FACTORS(...) \
    if (modValue != NULL) \
        for (auto i = 0; i < NUM_MOD_MATRIX_SOURCES; i++) \
        { \
            MAC_EACH(APPLY_MODMATRIX_FACTOR, _, __VA_ARGS__) \
        }


class ModulationSource {
    
    
    public:
    ModulationSource();
    float getDestinationFactor(int location, int parameter);
    void setDestinationFactor(int location, int parameter, float value);
    
    // public for ports:
    float destination[NUM_MOD_MATRIX_DESTINATIONS];
    
    static const rtosc::Ports ports;
    
    private:
    int getIndex(int location, int parameter);
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
