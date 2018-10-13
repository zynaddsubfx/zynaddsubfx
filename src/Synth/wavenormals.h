/*
  ZynAddSubFX - a software synthesizer

  wavenormals.h - definition of wave normal table
  Copyright (C) 2016-2018 Johannes Lorenz

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef WAVENORMALS_H
#define WAVENORMALS_H

//! returns an array:
//! basefunc i with parameter j is in array [i-1][j<<2]
//! all values in between are for parameters between two ints
const float (*getWavenormals())[512];

#endif // WAVENORMALS_H
