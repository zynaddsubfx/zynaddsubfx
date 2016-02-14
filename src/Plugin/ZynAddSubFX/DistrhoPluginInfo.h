/*
  ZynAddSubFX - a software synthesizer

  DistrhoPluginInfo.h - DPF information header
  Copyright (C) 2015-2016 Filipe Coelho
  Author: Filipe Coelho

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

*/

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND "ZynAddSubFX"
#define DISTRHO_PLUGIN_NAME  "ZynAddSubFX"
#define DISTRHO_PLUGIN_URI   "http://zynaddsubfx.sourceforge.net"

#if defined(NTK_GUI)
 #define DISTRHO_PLUGIN_HAS_UI          1
 #define DISTRHO_PLUGIN_HAS_EMBED_UI    1
 #define DISTRHO_PLUGIN_HAS_EXTERNAL_UI 1
#elif defined(FLTK_GUI)
 #define DISTRHO_PLUGIN_HAS_UI          1
 #define DISTRHO_PLUGIN_HAS_EMBED_UI    0
 #define DISTRHO_PLUGIN_HAS_EXTERNAL_UI 1
#else
 #define DISTRHO_PLUGIN_HAS_UI          0
#endif

#define DISTRHO_PLUGIN_IS_RT_SAFE       1
#define DISTRHO_PLUGIN_IS_SYNTH         1
#define DISTRHO_PLUGIN_NUM_INPUTS       0
#define DISTRHO_PLUGIN_NUM_OUTPUTS      2
#define DISTRHO_PLUGIN_WANT_PROGRAMS    1
#define DISTRHO_PLUGIN_WANT_STATE       1
#define DISTRHO_PLUGIN_WANT_FULL_STATE  1

enum Parameters {
    kParamOscPort,
    kParamCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
