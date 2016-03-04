/*
  ZynAddSubFX - a software synthesizer

  DSSIControl.h - DSSI control ports "controller"
  Copyright (C) 2015 Olivier Jolly
  Author: Olivier Jolly

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef ZYNADDSUBFX_DSSICONTROL_H
#define ZYNADDSUBFX_DSSICONTROL_H


#include "DSSIControlDescription.h"

struct DSSIControl {

    DSSIControlDescription description;

    float *data; /// pointer to the value for this controller which is updated by the DSSI host

    /**
     * Ctr for a DSSIControl based on a DSSIControl description
     * @param controller_code the controller code
     * @param name the human readable code name
     */
    DSSIControl(DSSIControlDescription description) : description(description) { }

    /**
     * update the current control to the Master in charge of dispatching them to the parts, effects, ...
     * @param master the controller master in charge of further dispatch
     */
    void forward_control(Master *master);

    /**
     * scale the incoming value refereced by data in the hinted range to one expected by the Master dispatcher.
     * Boolean are toggled to 0 or 127, ...
     */
    int get_scaled_data() {
        if (LADSPA_IS_HINT_TOGGLED(description.port_range_hint.HintDescriptor)) {
            return *data <= 0 ? 0 : 127;
        } else if (description.port_range_hint.UpperBound < 127) {
            // when not using 127 or 128 as upper bound, scale the input using the port range hint to 0 .. 128
            return 128 * (*data - description.port_range_hint.LowerBound) /
                   (description.port_range_hint.UpperBound - description.port_range_hint.LowerBound);
        } else {
            return *data;
        }
    }

};

#endif //ZYNADDSUBFX_DSSICONTROL_H
