/*
  ZynAddSubFX - a software synthesizer

  DSSIControlDescription.cpp - Description of DSSI control ports
  Copyright (C) 2015 Olivier Jolly
  Author: Olivier Jolly

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "DSSIControlDescription.h"

// Mapping from dssi control index to midi CC
const DSSIControlDescription dssi_control_description[DSSIControlDescription::MAX_DSSI_CONTROLS] = {
    { C_modwheel, "Modwheel", {LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MIDDLE, 1, 127 }},
    { C_volume, "Volume", {LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MAXIMUM, 1, 127 }},
    { C_panning, "Panning"},
    { C_expression, "Expression", {LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MAXIMUM, 1, 127 }},
    { C_sustain, "Sustain", {LADSPA_HINT_TOGGLED| LADSPA_HINT_DEFAULT_0, 0, 1}},
    { C_portamento, "Portamento", {LADSPA_HINT_TOGGLED| LADSPA_HINT_DEFAULT_0, 0, 1}},
    { C_filterq, "Filter Q", {LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MIDDLE, 0, 128 }},
    { C_filtercutoff, "Filter cutoff", {LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_0, -1, 1 }},
    { C_bandwidth, "Bandwidth"},
    { C_fmamp, "FM amplification", {LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MAXIMUM, 1, 127 }},
    { C_resonance_center, "Renonance center", {LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_0, -1, 1 }},
    { C_resonance_bandwidth, "Resonance bandwidth", {LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_0, -1, 1 }},
};
