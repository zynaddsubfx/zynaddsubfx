/*
  ZynAddSubFX - a software synthesizer

  Distortion.cpp - DPF + Zyn Plugin for Distortion
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
#include "Effects/Distorsion.h"

/* ------------------------------------------------------------------------------------------------------------
 * Distortion plugin class */

class DistortionPlugin : public AbstractPluginFX<Distorsion>
{
public:
    DistortionPlugin()
        : AbstractPluginFX(11, 6) {}

protected:
   /* --------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin label.
      This label is a short restricted name consisting of only _, a-z, A-Z and 0-9 characters.
    */
    const char* getLabel() const noexcept override
    {
        return "Distortion";
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
        return d_cconst('Z', 'X', 'd', 's');
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
            parameter.name   = "L/R Cross";
            parameter.symbol = "lrcross";
            parameter.ranges.def = 35.0f;
            break;
        case 1:
            parameter.name   = "Drive";
            parameter.symbol = "drive";
            parameter.ranges.def = 56.0f;
            break;
        case 2:
            parameter.name   = "Level";
            parameter.symbol = "level";
            parameter.ranges.def = 70.0f;
            break;
        case 3:
            parameter.name   = "Type";
            parameter.symbol = "type";
            parameter.ranges.def = 0.0f;
            parameter.ranges.max = 13.0f;
            /*
            TODO: support for scalePoints in DPF
            scalePoints[ 0].label = "Arctangent";
            scalePoints[ 1].label = "Asymmetric";
            scalePoints[ 2].label = "Pow";
            scalePoints[ 3].label = "Sine";
            scalePoints[ 4].label = "Quantisize";
            scalePoints[ 5].label = "Zigzag";
            scalePoints[ 6].label = "Limiter";
            scalePoints[ 7].label = "Upper Limiter";
            scalePoints[ 8].label = "Lower Limiter";
            scalePoints[ 9].label = "Inverse Limiter";
            scalePoints[10].label = "Clip";
            scalePoints[11].label = "Asym2";
            scalePoints[12].label = "Pow2";
            scalePoints[13].label = "Sigmoid";
            scalePoints[ 0].value = 0.0f;
            scalePoints[ 1].value = 1.0f;
            scalePoints[ 2].value = 2.0f;
            scalePoints[ 3].value = 3.0f;
            scalePoints[ 4].value = 4.0f;
            scalePoints[ 5].value = 5.0f;
            scalePoints[ 6].value = 6.0f;
            scalePoints[ 7].value = 7.0f;
            scalePoints[ 8].value = 8.0f;
            scalePoints[ 9].value = 9.0f;
            scalePoints[10].value = 10.0f;
            scalePoints[11].value = 11.0f;
            scalePoints[12].value = 12.0f;
            scalePoints[13].value = 13.0f;
            */
            break;
        case 4:
            parameter.hints |= kParameterIsBoolean;
            parameter.name   = "Negate";
            parameter.symbol = "negate";
            parameter.ranges.def = 0.0f;
            parameter.ranges.max = 1.0f;
            break;
        case 5:
            parameter.name   = "Low-Pass Filter";
            parameter.symbol = "lpf";
            parameter.ranges.def = 96.0f;
            break;
        case 6:
            parameter.name   = "High-Pass Filter";
            parameter.symbol = "hpf";
            parameter.ranges.def = 0.0f;
            break;
        case 7:
            parameter.hints |= kParameterIsBoolean;
            parameter.name   = "Stereo";
            parameter.symbol = "stereo";
            parameter.ranges.def = 0.0f;
            parameter.ranges.max = 1.0f;
            break;
        case 8:
            parameter.hints |= kParameterIsBoolean;
            parameter.name   = "Pre-Filtering";
            parameter.symbol = "pf";
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
            programName = "Overdrive 1";
            break;
        case 1:
            programName = "Overdrive 2";
            break;
        case 2:
            programName = "A. Exciter 1";
            break;
        case 3:
            programName = "A. Exciter 2";
            break;
        case 4:
            programName = "Guitar Amp";
            break;
        case 5:
            programName = "Quantisize";
            break;
        }
    }

    DISTRHO_DECLARE_NON_COPY_CLASS(DistortionPlugin)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create plugin, entry point */

START_NAMESPACE_DISTRHO

Plugin* createPlugin()
{
    return new DistortionPlugin();
}

END_NAMESPACE_DISTRHO
