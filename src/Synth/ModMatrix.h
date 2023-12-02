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

//~ #define NUM_MODMATRIX_SOURCES 5
enum {
    MOD_ENV1,
    MOD_ENV2,
    MOD_LFO1,
    MOD_LFO2,
    NUM_MODMATRIX_SOURCES
};

enum {
    PAR_LFO_FREQ,
    PAR_LFO_DEPTH,
    NUM_MODMATRIX_LFO_DESTINATIONS
};

/**Class for creating Low Frequency Oscillators*/
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

        float* value;
        float*** matrix;
        
        static const rtosc::Ports ports;

    private:

    
};

}

#endif
