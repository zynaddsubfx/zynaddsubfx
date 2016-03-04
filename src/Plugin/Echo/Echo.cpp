/*
  ZynAddSubFX - a software synthesizer

  Echo.cpp - DPF + Zyn Plugin for Echo
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
#include "Effects/Echo.h"

/* ------------------------------------------------------------------------------------------------------------
 * Echo plugin class */

class EchoPlugin : public AbstractPluginFX<Echo>
{
public:
    EchoPlugin()
        : AbstractPluginFX(7, 9) {}

protected:
   /* --------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin label.
      This label is a short restricted name consisting of only _, a-z, A-Z and 0-9 characters.
    */
    const char* getLabel() const noexcept override
    {
        return "Echo";
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
        return d_cconst('Z', 'X', 'e', 'c');
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
            parameter.name   = "Delay";
            parameter.symbol = "delay";
            parameter.ranges.def = 35.0f;
            break;
        case 1:
            parameter.name   = "L/R Delay";
            parameter.symbol = "lrdelay";
            parameter.ranges.def = 64.0f;
            break;
        case 2:
            parameter.name   = "L/R Cross";
            parameter.symbol = "lrcross";
            parameter.ranges.def = 30.0f;
            break;
        case 3:
            parameter.name   = "Feedback";
            parameter.symbol = "fb";
            parameter.ranges.def = 59.0f;
            break;
        case 4:
            parameter.name   = "High Damp";
            parameter.symbol = "damp";
            parameter.ranges.def = 0.0f;
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
            programName = "Echo 1";
            break;
        case 1:
            programName = "Echo 2";
            break;
        case 2:
            programName = "Echo 3";
            break;
        case 3:
            programName = "Simple Echo";
            break;
        case 4:
            programName = "Canyon";
            break;
        case 5:
            programName = "Panning Echo 1";
            break;
        case 6:
            programName = "Panning Echo 2";
            break;
        case 7:
            programName = "Panning Echo 3";
            break;
        case 8:
            programName = "Feedback Echo";
            break;
        }
    }

    DISTRHO_DECLARE_NON_COPY_CLASS(EchoPlugin)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create plugin, entry point */

START_NAMESPACE_DISTRHO

Plugin* createPlugin()
{
    return new EchoPlugin();
}

END_NAMESPACE_DISTRHO
