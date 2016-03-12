/*
  ZynAddSubFX - a software synthesizer

  AbstractFX.hpp - DPF Abstract Effect class
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
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

*/

#ifndef ZYNADDSUBFX_ABSTRACTFX_HPP_INCLUDED
#define ZYNADDSUBFX_ABSTRACTFX_HPP_INCLUDED

// DPF includes
#include "DistrhoPlugin.hpp"

// ZynAddSubFX includes
#include "Effects/Effect.h"
#include "Misc/Allocator.h"

/* ------------------------------------------------------------------------------------------------------------
 * Abstract plugin class */

template<class ZynFX>
class AbstractPluginFX : public Plugin
{
public:
    AbstractPluginFX(const uint32_t params, const uint32_t programs)
        : Plugin(params-2, programs, 0),
          paramCount(params-2), // volume and pan handled by host
          programCount(programs),
          bufferSize(getBufferSize()),
          sampleRate(getSampleRate()),
          effect(nullptr),
          efxoutl(nullptr),
          efxoutr(nullptr)
    {
        efxoutl = new float[bufferSize];
        efxoutr = new float[bufferSize];
        std::memset(efxoutl, 0, sizeof(float)*bufferSize);
        std::memset(efxoutr, 0, sizeof(float)*bufferSize);

        doReinit(true);
    }

    ~AbstractPluginFX() override
    {
        delete[] efxoutl;
        delete[] efxoutr;
        delete effect;
    }

protected:
   /* --------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin author/maker.
    */
    const char* getMaker() const noexcept override
    {
        return "ZynAddSubFX Team";
    }

   /**
      Get the plugin homepage.
      Optional, returns nothing by default.
    */
    const char* getHomePage() const noexcept override
    {
        return "http://zynaddsubfx.sourceforge.net";
    }

   /**
      Get the plugin license (a single line of text or a URL).
    */
    const char* getLicense() const noexcept override
    {
        return "GPL v2";
    }

   /**
      Get the plugin version, in hexadecimal.
    */
    uint32_t getVersion() const noexcept override
    {
        // TODO: use config.h or globals.h
        return d_version(2, 5, 4);
    }

   /* --------------------------------------------------------------------------------------------------------
    * Internal data */

   /**
      Get the current value of a parameter.
      The host may call this function from any context, including realtime processing.
    */
    float getParameterValue(uint32_t index) const override
    {
        return static_cast<float>(effect->getpar(static_cast<int>(index+2)));
    }

   /**
      Change a parameter value.
      The host may call this function from any context, including realtime processing.
      When a parameter is marked as automable, you must ensure no non-realtime operations are performed.
      @note This function will only be called for parameter inputs.
    */
    void setParameterValue(uint32_t index, float value) override
    {
        // make sure value is in bounds for uchar conversion
        if (value < 0.0f)
            value = 0.0f;
        else if (value > 127.0f)
            value = 127.0f;

        // safely cast to int
        const int ivalue = (int)(value+0.5f);

        // ok!
        effect->changepar(static_cast<int>(index+2), static_cast<uchar>(ivalue));
    }

   /**
      Load a program.
      The host may call this function from any context, including realtime processing.
    */
    void loadProgram(uint32_t index) override
    {
        effect->setpreset(static_cast<uchar>(index));

        // reset volume and pan
        effect->changepar(0, 127);
        effect->changepar(1, 64);
    }

   /* --------------------------------------------------------------------------------------------------------
    * Audio/MIDI Processing */

   /**
      Activate this plugin.
    */
    void activate() override
    {
        effect->cleanup();
    }

   /**
      Run/process function for plugins without MIDI input.
      @note Some parameters might be null if there are no audio inputs or outputs.
    */
    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        if (outputs[0] != inputs[0])
            copyWithMultiply(outputs[0], inputs[0], 0.5f, frames);
        else
            multiply(outputs[0], 0.5f, frames);

        if (outputs[1] != inputs[1])
            copyWithMultiply(outputs[1], inputs[1], 0.5f, frames);
        else
            multiply(outputs[1], 0.5f, frames);

        // FIXME: Make Zyn use const floats
        effect->out(Stereo<float*>((float*)inputs[0], (float*)inputs[1]));

        addWithMultiply(outputs[0], efxoutl, 0.5f, frames);
        addWithMultiply(outputs[1], efxoutr, 0.5f, frames);
    }

   /* --------------------------------------------------------------------------------------------------------
    * Callbacks (optional) */

   /**
      Optional callback to inform the plugin about a buffer size change.
      This function will only be called when the plugin is deactivated.
      @note This value is only a hint!
            Hosts might call run() with a higher or lower number of frames.
    */
    void bufferSizeChanged(uint32_t newBufferSize) override
    {
        if (bufferSize == newBufferSize)
            return;

        bufferSize = newBufferSize;

        delete[] efxoutl;
        delete[] efxoutr;
        efxoutl = new float[bufferSize];
        efxoutr = new float[bufferSize];
        std::memset(efxoutl, 0, sizeof(float)*bufferSize);
        std::memset(efxoutr, 0, sizeof(float)*bufferSize);

        doReinit(false);
    }

   /**
      Optional callback to inform the plugin about a sample rate change.
      This function will only be called when the plugin is deactivated.
    */
    void sampleRateChanged(double newSampleRate) override
    {
        if (sampleRate == newSampleRate)
            return;

        sampleRate = newSampleRate;

        doReinit(false);
    }

    // -------------------------------------------------------------------------------------------------------

private:
    const uint32_t paramCount;
    const uint32_t programCount;

    uint32_t bufferSize;
    double   sampleRate;

    Effect* effect;
    float*  efxoutl;
    float*  efxoutr;

    AllocatorClass allocator;

    void doReinit(const bool firstInit)
    {
        // save current param values before recreating effect
        uchar params[paramCount];

        if (effect != nullptr)
        {
            for (int i=0, count=static_cast<int>(paramCount); i<count; ++i)
                params[i] = effect->getpar(i+2);

            delete effect;
        }

        EffectParams pars(allocator, false, efxoutl, efxoutr, 0, static_cast<uint>(sampleRate), static_cast<int>(bufferSize));
        effect = new ZynFX(pars);

        if (firstInit)
        {
            effect->setpreset(0);
        }
        else
        {
            for (int i=0, count=static_cast<int>(paramCount); i<count; ++i)
                effect->changepar(i+2, params[i]);
        }

        // reset volume and pan
        effect->changepar(0, 127);
        effect->changepar(1, 64);
    }

    void addWithMultiply(float* dst, const float* src, const float multiplier, const uint32_t frames) noexcept
    {
        for (uint32_t i=0; i<frames; ++i)
            dst[i] += src[i] * multiplier;
    }

    void copyWithMultiply(float* dst, const float* src, const float multiplier, const uint32_t frames) noexcept
    {
        for (uint32_t i=0; i<frames; ++i)
            dst[i] = src[i] * multiplier;
    }

    void multiply(float* dst, const float multiplier, const uint32_t frames) noexcept
    {
        for (uint32_t i=0; i<frames; ++i)
            dst[i] *= multiplier;
    }

    DISTRHO_DECLARE_NON_COPY_CLASS(AbstractPluginFX)
};

#endif // ZYNADDSUBFX_ABSTRACTFX_HPP_INCLUDED
