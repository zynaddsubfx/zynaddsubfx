/*
  ZynAddSubFX - a software synthesizer

  DistrhoPluginInfo.h - DPF information header
  Copyright (C) 2015-2016 Filipe Coelho
  Author: Filipe Coelho

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
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
#elif defined(ZEST_GUI)
 #define DGL_OPENGL                     1
 #define HAVE_OPENGL                    1
 #define DISTRHO_PLUGIN_HAS_UI          1
 #define DISTRHO_PLUGIN_HAS_EMBED_UI    1
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
    kParamSlot1,
    kParamSlot2,
    kParamSlot3,
    kParamSlot4,
    kParamSlot5,
    kParamSlot6,
    kParamSlot7,
    kParamSlot8,
    kParamSlot9,
    kParamSlot10,
    kParamSlot11,
    kParamSlot12,
    kParamSlot13,
    kParamSlot14,
    kParamSlot15,
    kParamSlot16,
    kParamOscPort,
    kParamCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
