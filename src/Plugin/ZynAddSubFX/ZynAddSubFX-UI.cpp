/*
  ZynAddSubFX - a software synthesizer

  ZynAddSubFX-UI.cpp - DPF + ZynAddSubFX External UI
  Copyright (C) 2015-2016 Filipe Coelho
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

// DPF includes
#include "DistrhoUI.hpp"

/* ------------------------------------------------------------------------------------------------------------
 * ZynAddSubFX UI class */

class ZynAddSubFXUI : public UI
{
public:
    ZynAddSubFXUI(const intptr_t wid)
        : UI(390, 525),
          oscPort(0),
          winId(wid)
    {
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
    }

private:
    int oscPort;
    const intptr_t winId;

    void respawnAtURL(const int url)
    {
        char urlAsString[32];
        sprintf(urlAsString, "osc.udp://localhost:%i/", url);

        char winIdAsString[32];
        sprintf(winIdAsString, "%llu", (long long unsigned)winId);

        printf("Now respawning at '%s', using winId '%s'\n", urlAsString, winIdAsString);

        const char* args[] = {
            "zynaddsubfx-ext-gui",
            "--embed",
            winIdAsString,
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
    return new ZynAddSubFXUI(winId);
}

END_NAMESPACE_DISTRHO
