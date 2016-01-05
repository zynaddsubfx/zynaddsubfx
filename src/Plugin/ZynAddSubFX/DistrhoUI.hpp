/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_UI_HPP_INCLUDED
#define DISTRHO_UI_HPP_INCLUDED

#include "extra/LeakDetector.hpp"

START_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * DPF UI */

/**
   DPF UI class from where UI instances are created.
 */
class UI
{
public:
   /**
      UI class constructor.
      The UI should be initialized to a default state that matches the plugin side.
    */
    UI();

   /**
      Destructor.
    */
    virtual ~UI();

   /* --------------------------------------------------------------------------------------------------------
    * Host state */

   /**
      editParameter.
    */
    void editParameter(uint32_t index, bool started);

   /**
      setParameterValue.
    */
    void setParameterValue(uint32_t index, float value);

   /**
      setState.
    */
    void setState(const char* key, const char* value);

   /**
      sendNote.
    */
    void sendNote(uint8_t channel, uint8_t note, uint8_t velocity);

protected:
   /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

   /**
      A parameter has changed on the plugin side.@n
      This is called by the host to inform the UI about parameter changes.
    */
    virtual void parameterChanged(uint32_t index, float value) = 0;

   /**
      A program has been loaded on the plugin side.@n
      This is called by the host to inform the UI about program changes.
    */
    virtual void programLoaded(uint32_t index) = 0;

   /**
      A state has changed on the plugin side.@n
      This is called by the host to inform the UI about state changes.
    */
    virtual void stateChanged(const char* key, const char* value) = 0;

    // -------------------------------------------------------------------------------------------------------

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class UIExporter;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UI)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create UI, entry point */

/**
   createUI.
 */
extern UI* createUI(const intptr_t winId);

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_HPP_INCLUDED
