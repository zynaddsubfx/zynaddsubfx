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

#ifndef DISTRHO_UI_INTERNAL_HPP_INCLUDED
#define DISTRHO_UI_INTERNAL_HPP_INCLUDED

#include "DistrhoUI.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// Static data, see DistrhoUI.cpp

extern double    d_lastUiSampleRate;
extern uintptr_t g_nextWindowId;

// -----------------------------------------------------------------------
// UI callbacks

typedef void (*editParamFunc) (void* ptr, uint32_t rindex, bool started);
typedef void (*setParamFunc)  (void* ptr, uint32_t rindex, float value);
typedef void (*setStateFunc)  (void* ptr, const char* key, const char* value);
typedef void (*sendNoteFunc)  (void* ptr, uint8_t channel, uint8_t note, uint8_t velo);
typedef void (*setSizeFunc)   (void* ptr, uint width, uint height);

// -----------------------------------------------------------------------
// UI private data

struct UI::PrivateData {
    // DSP
    uint32_t parameterOffset;

    // Callbacks
    editParamFunc editParamCallbackFunc;
    setParamFunc  setParamCallbackFunc;
    setStateFunc  setStateCallbackFunc;
    sendNoteFunc  sendNoteCallbackFunc;
    setSizeFunc   setSizeCallbackFunc;
    void*         ptr;

    PrivateData() noexcept
        : parameterOffset(0),
          editParamCallbackFunc(nullptr),
          setParamCallbackFunc(nullptr),
          setStateCallbackFunc(nullptr),
          sendNoteCallbackFunc(nullptr),
          setSizeCallbackFunc(nullptr),
          ptr(nullptr)
    {
#ifdef DISTRHO_PLUGIN_TARGET_LV2
        // events in+out
        parameterOffset += 4 /* 2 stereo outs + events in&out */;
#endif
    }

    void editParamCallback(const uint32_t rindex, const bool started)
    {
        if (editParamCallbackFunc != nullptr)
            editParamCallbackFunc(ptr, rindex, started);
    }

    void setParamCallback(const uint32_t rindex, const float value)
    {
        if (setParamCallbackFunc != nullptr)
            setParamCallbackFunc(ptr, rindex, value);
    }

    void setStateCallback(const char* const key, const char* const value)
    {
        if (setStateCallbackFunc != nullptr)
            setStateCallbackFunc(ptr, key, value);
    }

    void sendNoteCallback(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        if (sendNoteCallbackFunc != nullptr)
            sendNoteCallbackFunc(ptr, channel, note, velocity);
    }

    void setSizeCallback(const uint width, const uint height)
    {
        if (setSizeCallbackFunc != nullptr)
            setSizeCallbackFunc(ptr, width, height);
    }
};

// -----------------------------------------------------------------------
// UI exporter class

static UI* createWindowIdUI(const intptr_t winId)
{
    g_nextWindowId = winId;
    UI* const ui(createUI());
    g_nextWindowId = 0;
    return ui;
}

class UIExporter
{
public:
    UIExporter(void* const ptr, const intptr_t winId,
               const editParamFunc editParamCall, const setParamFunc setParamCall, const setStateFunc setStateCall, const sendNoteFunc sendNoteCall, const setSizeFunc setSizeCall,
               void* const dspPtr = nullptr)
        : fUI(createWindowIdUI(winId)),
          fData((fUI != nullptr) ? fUI->pData : nullptr)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);

        fData->ptr = ptr;
        fData->editParamCallbackFunc = editParamCall;
        fData->setParamCallbackFunc  = setParamCall;
        fData->setStateCallbackFunc  = setStateCall;
        fData->sendNoteCallbackFunc  = sendNoteCall;
        fData->setSizeCallbackFunc   = setSizeCall;
    }

    // -------------------------------------------------------------------

    uint32_t getParameterOffset() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);

        return fData->parameterOffset;
    }

    // -------------------------------------------------------------------

    void parameterChanged(const uint32_t index, const float value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        fUI->parameterChanged(index, value);
    }

    void programLoaded(const uint32_t index)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        fUI->programLoaded(index);
    }

    void stateChanged(const char* const key, const char* const value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        DISTRHO_SAFE_ASSERT_RETURN(value != nullptr,);

        fUI->stateChanged(key, value);
    }

    // -------------------------------------------------------------------
    // compatibility calls, used for regular OpenGL windows

    uint getWidth() const noexcept
    {
        return 390;
    }

    uint getHeight() const noexcept
    {
        return 525;
    }

    intptr_t getWindowId() const noexcept
    {
        return 0;
    }

    bool isVisible() const noexcept
    {
        return true;
    }

    void setSampleRate(const double sampleRate, const bool doCallback = false)
    {
    }

    void setWindowSize(const uint width, const uint height, const bool updateUI = false)
    {
    }

    void setWindowTitle(const char* const uiTitle)
    {
    }

    void setWindowTransientWinId(const uintptr_t winId)
    {
    }

    bool setWindowVisible(const bool yesNo)
    {
        return true;
    }

    bool idle()
    {
        return true;
    }

    void quit()
    {
    }

private:
    // -------------------------------------------------------------------

    UI* const fUI;
    UI::PrivateData* const fData;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIExporter)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_INTERNAL_HPP_INCLUDED
