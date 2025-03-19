/*
  ZynAddSubFX - a software synthesizer

  Reverse.cpp - DPF + Zyn Plugin for Reverse
  Copyright (C) 2015 Filipe Coelho
  Author: Filipe Coelho

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

// DPF includes
#include "../AbstractFX.hpp"

// ZynAddSubFX includes
#include "Effects/Reverse.h"

/* ------------------------------------------------------------------------------------------------------------
 * Reverse plugin class */

class ReversePlugin : public AbstractPluginFX<zyn::Reverse>
{
public:
    ReversePlugin()
        : AbstractPluginFX(6, 5) {}

protected:
   /* --------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin label.
      This label is a short restricted name consisting of only _, a-z, A-Z and 0-9 characters.
    */
    const char* getLabel() const noexcept override
    {
        return "Reverse";
    }

   /**
      Get an extensive comment/description about the plugin.
    */
    const char* getDescription() const noexcept override
    {
        // TODO
        return "";
    }

   /**
      Get the plugin unique Id.
      This value is used by LADSPA, DSSI and VST plugin formats.
    */
    int64_t getUniqueId() const noexcept override
    {
        return d_cconst('Z', 'X', 'r', 's');
    }

    zyn::Reverse* instantiateFX(zyn::EffectParams pars) override
    {
        return new zyn::Reverse(pars, &time);
    }

   /* --------------------------------------------------------------------------------------------------------
    * Init */

   /**
      Initialize the parameter @a index.
      This function will be called once, shortly after the plugin is created.
    */
void initParameter(uint32_t index, Parameter& parameter) noexcept override
{
    parameter.hints = kParameterIsInteger | kParameterIsAutomable; // Alle Parameter automatisierbar
    parameter.unit  = "";
    parameter.ranges.min = 0.0f;
    parameter.ranges.max = 127.0f;
    parameter.ranges.def = 64.0f; // Standardwert für die meisten Parameter

    switch (index)
    {
    case 0: // Delay
        parameter.name   = "Delay";
        parameter.symbol = "delay";
        break;
    case 1: // Stereo
        parameter.name   = "Stereo";
        parameter.symbol = "stereo";
        parameter.ranges.max = 1.0f;
        parameter.ranges.def = 0.0f;
        break;
    case 2: // Phase
        parameter.name   = "Phase";
        parameter.symbol = "phase";
        break;
    case 3: // Crossfade
        parameter.name   = "Crossfade";
        parameter.symbol = "crossfade";
        break;
    case 4: // Sync Mode
        parameter.name   = "Sync Mode";
        parameter.symbol = "syncMode";
        break;
    default:
        // For unused parameters
        parameter.name   = "Unused";
        parameter.symbol = "unused";
        break;
    }
}

   /**
      Set the name of the program @a index.
      This function will be called once, shortly after the plugin is created.
    */
void initProgramName(uint32_t index, String& programName) noexcept override
{
    switch (index)
    {
    case 0:
        programName = "Note On";
        break;
    case 1:
        programName = "Note On/Off";
        break;
    case 2:
        programName = "Auto";
        break;
    default:
        programName = "Custom Preset"; // Für alle anderen Presets
        break;
    }
}

    DISTRHO_DECLARE_NON_COPY_CLASS(ReversePlugin)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create plugin, entry point */

START_NAMESPACE_DISTRHO

Plugin* createPlugin()
{
    Plugin* plugin = new ReversePlugin();
    ReversePlugin* fxPlugin = dynamic_cast<ReversePlugin*>(plugin);
    if (fxPlugin)
    {
        fxPlugin->doReinit(true);
    }
    return plugin;
}

END_NAMESPACE_DISTRHO
