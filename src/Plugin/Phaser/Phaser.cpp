/*
  ZynAddSubFX - a software synthesizer

  Phaser.cpp - DPF + Zyn Plugin for Phaser
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
#include "Effects/Phaser.h"

/* ------------------------------------------------------------------------------------------------------------
 * Phaser plugin class */

class PhaserPlugin : public AbstractPluginFX<Phaser>
{
public:
    PhaserPlugin()
        : AbstractPluginFX(15, 12) {}

protected:
   /* --------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin label.
      This label is a short restricted name consisting of only _, a-z, A-Z and 0-9 characters.
    */
    const char* getLabel() const noexcept override
    {
        return "Phaser";
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
        return d_cconst('Z', 'X', 'p', 'h');
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
            parameter.ranges.def = 36.0f;
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
            parameter.ranges.def = 64.0f;
            break;
        case 4:
            parameter.name   = "Depth";
            parameter.symbol = "depth";
            parameter.ranges.def = 110.0f;
            break;
        case 5:
            parameter.name = "Feedback";
            parameter.symbol = "fb";
            parameter.ranges.def = 64.0f;
            break;
        case 6:
            parameter.name   = "Stages";
            parameter.symbol = "stages";
            parameter.ranges.def = 1.0f;
            parameter.ranges.min = 1.0f;
            parameter.ranges.max = 12.0f;
            break;
        case 7:
            parameter.name   = "L/R Cross|Offset";
            parameter.symbol = "lrcross";
            parameter.ranges.def = 0.0f;
            break;
        case 8:
            parameter.hints |= kParameterIsBoolean;
            parameter.name   = "Subtract Output";
            parameter.symbol = "subsout";
            parameter.ranges.def = 0.0f;
            parameter.ranges.max = 1.0f;
            break;
        case 9:
            parameter.name   = "Phase|Width";
            parameter.symbol = "phase";
            parameter.ranges.def = 20.0f;
            break;
        case 10:
            parameter.hints |= kParameterIsBoolean;
            parameter.name   = "Hyper";
            parameter.symbol = "hyper";
            parameter.ranges.def = 0.0f;
            parameter.ranges.max = 1.0f;
            break;
        case 11:
            parameter.name   = "Distortion";
            parameter.symbol = "dist";
            parameter.ranges.def = 0.0f;
            break;
        case 12:
            parameter.hints |= kParameterIsBoolean;
            parameter.name   = "Analog";
            parameter.symbol = "analog";
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
            programName = "Phaser 1";
            break;
        case 1:
            programName = "Phaser 2";
            break;
        case 2:
            programName = "Phaser 3";
            break;
        case 3:
            programName = "Phaser 4";
            break;
        case 4:
            programName = "Phaser 5";
            break;
        case 5:
            programName = "Phaser 6";
            break;
        case 6:
            programName = "Analog Phaser 1";
            break;
        case 7:
            programName = "Analog Phaser 2";
            break;
        case 8:
            programName = "Analog Phaser 3";
            break;
        case 9:
            programName = "Analog Phaser 4";
            break;
        case 10:
            programName = "Analog Phaser 5";
            break;
        case 11:
            programName = "Analog Phaser 6";
            break;
        }
    }

    DISTRHO_DECLARE_NON_COPY_CLASS(PhaserPlugin)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create plugin, entry point */

START_NAMESPACE_DISTRHO

Plugin* createPlugin()
{
    return new PhaserPlugin();
}

END_NAMESPACE_DISTRHO
