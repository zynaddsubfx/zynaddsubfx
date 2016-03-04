/*
  ZynAddSubFX - a software synthesizer

  Reverb.cpp - DPF + Zyn Plugin for Reverb
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
#include "Effects/Reverb.h"

/* ------------------------------------------------------------------------------------------------------------
 * Reverb plugin class */

class ReverbPlugin : public AbstractPluginFX<Reverb>
{
public:
    ReverbPlugin()
        : AbstractPluginFX(13, 13) {}

protected:
   /* --------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin label.
      This label is a short restricted name consisting of only _, a-z, A-Z and 0-9 characters.
    */
    const char* getLabel() const noexcept override
    {
        return "Reverb";
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
        return d_cconst('Z', 'X', 'r', 'v');
    }

   /* --------------------------------------------------------------------------------------------------------
    * Init */

   /**
      Initialize the parameter @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initParameter(uint32_t index, Parameter& parameter) noexcept override
    {
        parameter.hints = kParameterIsInteger;
        parameter.unit  = "";
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 127.0f;

        switch (index)
        {
        case 0:
            parameter.hints |= kParameterIsAutomable;
            parameter.name   = "Time";
            parameter.symbol = "time";
            parameter.ranges.def = 63.0f;
            break;
        case 1:
            parameter.name   = "Delay";
            parameter.symbol = "delay";
            parameter.ranges.def = 24.0f;
            break;
        case 2:
            parameter.hints |= kParameterIsAutomable;
            parameter.name   = "Feedback";
            parameter.symbol = "fb";
            parameter.ranges.def = 0.0f;
            break;
        case 3:
            // FIXME: unused
            parameter.name   = "bw (unused)";
            parameter.symbol = "unused_bw";
            parameter.ranges.def = 0.0f;
            break;
        case 4:
            // FIXME: unused
            parameter.name   = "E/R (unused)";
            parameter.symbol = "unused_er";
            parameter.ranges.def = 0.0f;
            break;
        case 5:
            parameter.name   = "Low-Pass Filter";
            parameter.symbol = "lpf";
            parameter.ranges.def = 85.0f;
            break;
        case 6:
            parameter.name   = "High-Pass Filter";
            parameter.symbol = "hpf";
            parameter.ranges.def = 5.0f;
            break;
        case 7:
            parameter.hints |= kParameterIsAutomable;
            parameter.name   = "Damp";
            parameter.symbol = "damp";
            parameter.ranges.def = 83.0f;
            parameter.ranges.min = 64.0f;
            break;
        case 8:
            parameter.name   = "Type";
            parameter.symbol = "type";
            parameter.ranges.def = 1.0f;
            parameter.ranges.max = 2.0f;
            /*
            TODO: support for scalePoints in DPF
            scalePoints[0].label = "Random";
            scalePoints[1].label = "Freeverb";
            scalePoints[2].label = "Bandwidth";
            scalePoints[0].value = 0.0f;
            scalePoints[1].value = 1.0f;
            scalePoints[2].value = 2.0f;
            */
            break;
        case 9:
            parameter.name   = "Room size";
            parameter.symbol = "size";
            parameter.ranges.def = 64.0f;
            parameter.ranges.min = 1.0f;
            break;
        case 10:
            parameter.name   = "Bandwidth";
            parameter.symbol = "bw";
            parameter.ranges.def = 20.0f;
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
            programName = "Cathedral 1";
            break;
        case 1:
            programName = "Cathedral 2";
            break;
        case 2:
            programName = "Cathedral 3";
            break;
        case 3:
            programName = "Hall 1";
            break;
        case 4:
            programName = "Hall 2";
            break;
        case 5:
            programName = "Room 1";
            break;
        case 6:
            programName = "Room 2";
            break;
        case 7:
            programName = "Basement";
            break;
        case 8:
            programName = "Tunnel";
            break;
        case 9:
            programName = "Echoed 1";
            break;
        case 10:
            programName = "Echoed 2";
            break;
        case 11:
            programName = "Very Long 1";
            break;
        case 12:
            programName = "Very Long 2";
            break;
        }
    }

    DISTRHO_DECLARE_NON_COPY_CLASS(ReverbPlugin)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create plugin, entry point */

START_NAMESPACE_DISTRHO

Plugin* createPlugin()
{
    return new ReverbPlugin();
}

END_NAMESPACE_DISTRHO
