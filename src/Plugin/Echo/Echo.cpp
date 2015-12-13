/*
  ZynAddSubFX - a software synthesizer

  Echo.cpp - DPF + Zyn Plugin for Echo
  Copyright (C) 2015 Filipe Coelho
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
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

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
        parameter.name  = "";
        parameter.unit  = "";
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 127.0f;

        switch (index)
        {
        case 0:
            parameter.name = "Delay";
            parameter.ranges.def = 35.0f;
            break;
        case 1:
            parameter.name = "L/R Delay";
            parameter.ranges.def = 64.0f;
            break;
        case 2:
            parameter.name = "L/R Cross";
            parameter.ranges.def = 30.0f;
            break;
        case 3:
            parameter.name = "Feedback";
            parameter.ranges.def = 59.0f;
            break;
        case 4:
            parameter.name = "High Damp";
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
