/*
  ZynAddSubFX - a software synthesizer

  basefunctions.h - functions to get the base functions
  Copyright (C) 2016-2018 Johannes Lorenz

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef BASEFUNCTIONS_H
#define BASEFUNCTIONS_H

namespace zyn {

typedef float filter_func_t(unsigned int, float, float);
typedef float base_func_t(float, float);

typedef float (*base_func)(float, float);
base_func_t* getBaseFunction(unsigned char func);

typedef float (*filter_func)(unsigned int, float, float);
filter_func_t* getFilter(unsigned char func);

float rmsNormalOfBaseFunction(unsigned char func,
    int oscilsize, unsigned char basefuncpar); // TODO: swap params

}

#endif // BASEFUNCTIONS_H
