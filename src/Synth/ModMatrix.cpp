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
    
#define rObject ModMatrix
#define rBegin [](const char *msg, RtData &d) { rObject &o = *(rObject*)d.obj
#define rEnd }

//~ #define N_POINTS NUM_MODMATRIX_SOURCES*NUM_MODMATRIX_DESTINATIONS

const rtosc::Ports ModMatrix::ports = {
    rSelf(Matrix),
    //~ {"matrix", 0, 0,
        //~ rBegin;
        //~ if(rtosc_narguments(msg)) {
            //~ int i=0;
            //~ auto itr = rtosc_itr_begin(msg);
            //~ while(!rtosc_itr_end(itr) && i < N_POINTS) {
                //~ auto ival = rtosc_itr_next(&itr);
                //~ if(ival.type == 'f')
                    //~ o.matrix[i/NUM_MODMATRIX_DESTINATIONS][i%NUM_MODMATRIX_DESTINATIONS] = ival.val.f;
            //~ }
        //~ } else {
            //~ rtosc_arg_t args[N_POINTS];
            //~ char        types[N_POINTS+1] = {};
            //~ for(int i=0; i<N_POINTS; ++i) {
                //~ args[i].f = o.matrix[i/NUM_MODMATRIX_DESTINATIONS][i%NUM_MODMATRIX_DESTINATIONS];
                //~ types[i]  = 'f';
            //~ }
            //~ d.replyArray(d.loc, types, args);
        //~ }
        //~ rEnd},
};
#undef rBegin
#undef rEnd    
    
    

ModMatrix::ModMatrix()
{
    value = new float[NUM_MODMATRIX_SOURCES];
    matrix = new float**[NUM_MODMATRIX_SOURCES]; // Erstelle ein Array von Pointern
    
    for (int i = 0; i < NUM_MODMATRIX_SOURCES; ++i) {
        matrix[i] = new float*[NUM_LOCATIONS]; 
        for (int j = 0; j < NUM_LOCATIONS; ++j) {
            matrix[i][j] = new float[NUM_MODMATRIX_LFO_DESTINATIONS]; // Weise jedem Pointer ein Array zu
        }
    }
    

    memset(value, 0, NUM_MODMATRIX_SOURCES * sizeof(float));
    for (int i = 0; i < NUM_MODMATRIX_SOURCES; ++i) {
        for (int j = 0; j < NUM_LOCATIONS; ++j) {
            memset(matrix[i][j], 0, NUM_MODMATRIX_LFO_DESTINATIONS * sizeof(float));
        }
    }
}

ModMatrix::~ModMatrix()
{
    delete[] value;
    // Freigabe des Speichers für jedes Array, auf das 'matrix' zeigt
    for (int i = 0; i < NUM_MODMATRIX_SOURCES; ++i) {
        delete[] matrix[i];
    }
    
    delete[] matrix; // Freigabe des Speichers für das Array von Pointern auf Zeilen

}

}
