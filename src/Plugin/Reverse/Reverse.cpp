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
        : AbstractPluginFX(9, 5) {}

    void setSpeedfactor(float factor)
    {
        effect->speedfactor=factor;
    }

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
        return "Reverse is a host synchronized reverse delay effect with multiple trigger modes";
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


    /**
      Get the current value of a parameter.
      The host may call this function from any context, including realtime processing.
    */
    float getParameterValue(uint32_t index) const override
    {
        switch(index)
        {
            case 4:
                return AbstractPluginFX::getParameterValue(index)-2;
                break;
            case 5:
                return numerator;
                break;
            case 6:
                return denominator;
                break;
            default:
                return AbstractPluginFX::getParameterValue(index);
                break;
         }

    }

   /**
      Change a parameter value.
      The host may call this function from any context, including realtime processing.
      When a parameter is marked as automable, you must ensure no non-realtime operations are performed.
      @note This function will only be called for parameter inputs.
    */
    void setParameterValue(uint32_t index, float value) override
    {
        if(index==4)
        {
            AbstractPluginFX::setParameterValue(index, value+2);
            return;
        }

        if(index==5 || index==6)
        {
            if(index==5) numerator = value;
            if(index==6) denominator = value;
            if (numerator&&denominator)
                effect->speedfactor = (float)denominator / (4.0f *(float)numerator);
        }
        else
        {
            AbstractPluginFX::setParameterValue(index, value);
        }
    }



   /* --------------------------------------------------------------------------------------------------------
    * Init */

   /**
      Initialize the parameter @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initParameter(uint32_t index, Parameter& parameter) noexcept override
    {
        parameter.hints = kParameterIsInteger | kParameterIsAutomable; // All parameters automatable
        parameter.unit  = "";
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 127.0f;
        parameter.ranges.def = 64.0f; // default value for most parameters

        switch (index)
        {
        case 0: // Delay
            parameter.name   = "Delay";
            parameter.symbol = "delay";
            break;
        case 1: // Stereo
            parameter.hints |= kParameterIsBoolean;
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
            parameter.hints |= kParameterIsBoolean;
            parameter.name   = "Sync Mode";
            parameter.symbol = "syncMode";
            parameter.ranges.max = 1.0f; // SYNC
            parameter.ranges.def = 0.0f; // AUTO
            break;
        case 5: // Numerator
            parameter.name   = "Numerator of BPM ratio";
            parameter.symbol = "numerator";
            parameter.ranges.max = 32.0f;
            parameter.ranges.def = 0.0f;
            break;
        case 6: // Denominator
            parameter.name   = "Denominator of BPM ratio";
            parameter.symbol = "denominator";
            parameter.ranges.max = 32.0f;
            parameter.ranges.def = 4.0f;
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


private:
    float numerator = 0.0f;
    float denominator = 4.0f;


    DISTRHO_DECLARE_NON_COPY_CLASS(ReversePlugin)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create plugin, entry point */

START_NAMESPACE_DISTRHO

Plugin* createPlugin()
{
    ReversePlugin* plugin = new ReversePlugin();
    plugin->doReinit(true);
    plugin->setSpeedfactor(1.0f);
    return plugin;
}

END_NAMESPACE_DISTRHO
