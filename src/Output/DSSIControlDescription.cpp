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
    { zyn::C_modwheel, "Modwheel", {LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MIDDLE, 1, 127 }},
    { zyn::C_volume, "Volume", {LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MAXIMUM, 1, 127 }},
    { zyn::C_panning, "Panning"},
    { zyn::C_expression, "Expression", {LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MAXIMUM, 1, 127 }},
    { zyn::C_sustain, "Sustain", {LADSPA_HINT_TOGGLED| LADSPA_HINT_DEFAULT_0, 0, 1}},
    { zyn::C_portamento, "Portamento", {LADSPA_HINT_TOGGLED| LADSPA_HINT_DEFAULT_0, 0, 1}},
    { zyn::C_filterq, "Filter Q", {LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MIDDLE, 0, 128 }},
    { zyn::C_filtercutoff, "Filter cutoff", {LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_0, -1, 1 }},
    { zyn::C_bandwidth, "Bandwidth"},
    { zyn::C_fmamp, "FM amplification", {LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_MAXIMUM, 1, 127 }},
    { zyn::C_resonance_center, "Renonance center", {LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_0, -1, 1 }},
    { zyn::C_resonance_bandwidth, "Resonance bandwidth", {LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_DEFAULT_0, -1, 1 }},
};
