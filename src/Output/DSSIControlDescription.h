/*
  ZynAddSubFX - a software synthesizer

  DSSIControlDescription.h - Description of DSSI control ports
  Copyright (C) 2015 Olivier Jolly
  Author: Olivier Jolly

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef ZYNADDSUBFX_DSSICONTROLDESCRIPTION_H
#define ZYNADDSUBFX_DSSICONTROLDESCRIPTION_H


#include <globals.h>
#include <ladspa.h>

/**
 * DSSIControlDescription represent one instance of DSSI control used to describe accepted values to the DSSI host
 * and to forward DSSI host value change to ZynAddSubFx controller
 */
struct DSSIControlDescription {

    const static int MAX_DSSI_CONTROLS = 12;

    const MidiControllers controller_code; /// controler code, as accepted by the Controller class
    const char *name; /// human readable name of this control

    /** hint about usable range of value for this control, defaulting to 0-128, initially at 64 */
    const LADSPA_PortRangeHint port_range_hint = {
            LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE, 0, 128};

    /**
     * Ctr for a DSSIControlDescription using the default hint (0-128 starting at 64)
     * @param controller_code the controller code
     * @param name the human readable code name
     */
    DSSIControlDescription(MidiControllers controller_code, const char *name) : controller_code(controller_code),
                                                                                name(name) { }

    /**
     * Ctr for a DSSIControlDescription
     * @param controller_code the controller code
     * @param name the human readable code name
     * @param port_range_hint the accepted range of values
     */
    DSSIControlDescription(MidiControllers controller_code, const char *name, LADSPA_PortRangeHint port_range_hint) :
            controller_code(controller_code), name(name), port_range_hint(port_range_hint) { }

};

extern const DSSIControlDescription dssi_control_description[DSSIControlDescription::MAX_DSSI_CONTROLS];

#endif //ZYNADDSUBFX_DSSICONTROLDESCRIPTION_H
