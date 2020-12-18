/*
  ZynAddSubFX - a software synthesizer

  ZynAddSubFX-UI.cpp - DPF + ZynAddSubFX External UI
  Copyright (C) 2015-2016 Filipe Coelho
  Author: Filipe Coelho

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

// DPF includes
#include "DistrhoUI.hpp"

/* ------------------------------------------------------------------------------------------------------------
 * ZynAddSubFX UI class */

class ZynAddSubFXUI : public UI
{
public:
    ZynAddSubFXUI(const intptr_t wid, const char* const bpath)
        : UI(390, 525),
          oscPort(0),
          winId(wid)
    {
        setTitle("ZynAddSubFX");

#if DISTRHO_OS_MAC
        extUiPath  = bpath;
        extUiPath += "/zynaddsubfx-ext-gui";
#else
        extUiPath = "zynaddsubfx-ext-gui";

        // unused
        return; (void)bpath;
#endif
    }

    ~ZynAddSubFXUI() override
    {
    }

protected:
   /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

   /**
      A parameter has changed on the plugin side.
      This is called by the host to inform the UI about parameter changes.
    */
    void parameterChanged(uint32_t index, float value) override
    {
        switch (index)
        {
        case kParamOscPort: {
            const int port = int(value+0.5f);

            if (oscPort != port)
            {
                oscPort = port;
                respawnAtURL(port);
            }
        } break;
        }
    }

   /**
      A program has been loaded on the plugin side.
      This is called by the host to inform the UI about program changes.
    */
    void programLoaded(uint32_t index) override
    {
    }

   /**
      A state has changed on the plugin side.
      This is called by the host to inform the UI about state changes.
    */
    void stateChanged(const char* key, const char* value) override
    {
        (void)key; (void)value;
    }

private:
    int oscPort;
    String extUiPath;
    const intptr_t winId;

    void respawnAtURL(const int url)
    {
        char urlAsString[32];
        sprintf(urlAsString, "osc.udp://localhost:%i/", url);

        char winIdAsString[32];
        sprintf(winIdAsString, "%llu", (long long unsigned)(winId ? winId : 1));

        printf("Now respawning at '%s', using winId '%s'\n", urlAsString, winIdAsString);

        const char* args[] = {
            extUiPath.buffer(),
            "--embed",
            winIdAsString,
            "--title",
            getTitle(),
            urlAsString,
            nullptr
        };

        startExternalProcess(args);
    }

    DISTRHO_DECLARE_NON_COPY_CLASS(ZynAddSubFXUI)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create UI, entry point */

START_NAMESPACE_DISTRHO

UI* createUI()
{
#if DISTRHO_PLUGIN_HAS_EMBED_UI
    const uintptr_t winId = UI::getNextWindowId();
#else
    const uintptr_t winId = 0;
#endif
    const char* const bundlePath = UI::getNextBundlePath();

    return new ZynAddSubFXUI(winId, bundlePath);
}

END_NAMESPACE_DISTRHO
