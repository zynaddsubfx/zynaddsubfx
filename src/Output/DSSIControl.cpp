/*
  ZynAddSubFX - a software synthesizer

  DSSIControl.cpp - DSSI control ports "controller"
  Copyright (C) 2015 Olivier Jolly
  Author: Olivier Jolly

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include "DSSIControl.h"
#include "../Misc/Master.h"

void DSSIControl::forward_control(Master *master) {
    master->setController(0, description.controller_code, get_scaled_data());
}
