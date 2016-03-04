/*
  ZynAddSubFX - a software synthesizer

  DSSIControl.cpp - DSSI control ports "controller"
  Copyright (C) 2015 Olivier Jolly
  Author: Olivier Jolly

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "DSSIControl.h"
#include "../Misc/Master.h"

void DSSIControl::forward_control(Master *master) {
    master->setController(0, description.controller_code, get_scaled_data());
}
