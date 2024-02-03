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



#define MODMATRIX_DESTINATIONS \
    LFO1_FREQ,\
    LFO1_DEPTH,\
    LFO2_FREQ,\
    LFO2_DEPTH,\
    AMPLFO_FREQ,\
    AMPLFO_DEPTH,\
    FREQLFO_FREQ,\
    FREQLFO_DEPTH,\
    FILTLFO_FREQ,\
    FILTLFO_DEPTH\

#define NUM_MOD_MATRIX_DESTINATIONS 10

enum {
    MODMATRIX_DESTINATIONS
};

enum {
    PAR_LFO_FREQ,
    PAR_LFO_DEPTH,
    NUM_MODMATRIX_LFO_DESTINATIONS
};

#define NUM_MODMATRIX_ANY_DESTINATIONS 8

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
        unsigned char PDestinations;
        
        static const rtosc::Ports ports;
};

}

#endif
