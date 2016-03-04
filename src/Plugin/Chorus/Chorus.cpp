/*
  ZynAddSubFX - a software synthesizer

  Chorus.cpp - DPF + Zyn Plugin for Chorus
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
#include "Effects/Chorus.h"

/* ------------------------------------------------------------------------------------------------------------
 * Chorus plugin class */

class ChorusPlugin : public AbstractPluginFX<Chorus>
{
public:
    ChorusPlugin()
        : AbstractPluginFX(12, 10) {}

protected:
   /* --------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin label.
      This label is a short restricted name consisting of only _, a-z, A-Z and 0-9 characters.
    */
    const char* getLabel() const noexcept override
    {
        return "Chorus";
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
        return d_cconst('Z', 'X', 'c', 'h');
    }

   /* --------------------------------------------------------------------------------------------------------
    * Init */

   /**
      Initialize the parameter @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initParameter(uint32_t index, Parameter& parameter) noexcept override
    {
        parameter.hints = kParameterIsInteger|kParameterIsAutomable;
        parameter.unit  = "";
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 127.0f;

        switch (index)
        {
        case 0:
            parameter.name   = "LFO Frequency";
            parameter.symbol = "lfofreq";
            parameter.ranges.def = 50.0f;
            break;
        case 1:
            parameter.name   = "LFO Randomness";
            parameter.symbol = "lforand";
            parameter.ranges.def = 0.0f;
            break;
        case 2:
            parameter.name   = "LFO Type";
            parameter.symbol = "lfotype";
            parameter.ranges.def = 0.0f;
            parameter.ranges.max = 1.0f;
            /*
            TODO: support for scalePoints in DPF
            scalePoints[0].label = "Sine";
            scalePoints[1].label = "Triangle";
            scalePoints[0].value = 0.0f;
            scalePoints[1].value = 1.0f;
            */
            break;
        case 3:
            parameter.name   = "LFO Stereo";
            parameter.symbol = "lfostereo";
            parameter.ranges.def = 90.0f;
            break;
        case 4:
            parameter.name   = "Depth";
            parameter.symbol = "depth";
            parameter.ranges.def = 40.0f;
            break;
        case 5:
            parameter.name   = "Delay";
            parameter.symbol = "delay";
            parameter.ranges.def = 85.0f;
            break;
        case 6:
            parameter.name   = "Feedback";
            parameter.symbol = "fb";
            parameter.ranges.def = 64.0f;
            break;
        case 7:
            parameter.name   = "L/R Cross";
            parameter.symbol = "lrcross";
            parameter.ranges.def = 119.0f;
            break;
        case 8:
            parameter.hints |= kParameterIsBoolean;
            parameter.name   = "Flange Mode";
            parameter.symbol = "flang";
            parameter.ranges.def = 0.0f;
            parameter.ranges.max = 1.0f;
            break;
        case 9:
            parameter.hints |= kParameterIsBoolean;
            parameter.name   = "Subtract Output";
            parameter.symbol = "subsout";
            parameter.ranges.def = 0.0f;
            parameter.ranges.max = 1.0f;
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
            programName = "Chorus 1";
            break;
        case 1:
            programName = "Chorus 2";
            break;
        case 2:
            programName = "Chorus 3";
            break;
        case 3:
            programName = "Celeste 1";
            break;
        case 4:
            programName = "Celeste 2";
            break;
        case 5:
            programName = "Flange 1";
            break;
        case 6:
            programName = "Flange 2";
            break;
        case 7:
            programName = "Flange 3";
            break;
        case 8:
            programName = "Flange 4";
            break;
        case 9:
            programName = "Flange 5";
            break;
        }
    }

    DISTRHO_DECLARE_NON_COPY_CLASS(ChorusPlugin)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create plugin, entry point */

START_NAMESPACE_DISTRHO

Plugin* createPlugin()
{
    return new ChorusPlugin();
}

END_NAMESPACE_DISTRHO
