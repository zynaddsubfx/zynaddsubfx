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
#include "../../../DPF/dgl/Window.hpp"
#include "../../../DPF/dgl/Application.hpp"

/* ------------------------------------------------------------------------------------------------------------
 * ZynAddSubFX UI class */

class ZynAddSubFXUI : public UI
{
public:
    ZynAddSubFXUI(const intptr_t wid, const char* const bpath)
        : UI(390, 525),
          win(app),
          oscPort(0)
    {
        setTitle("ZynAddSubFX");
        printf("[INFO] Opened the zynaddsubfx UI...\n");

        win.setSize(100,200);
        win.show();

        (void) wid;
        (void) bpath;
    }

    ~ZynAddSubFXUI() override
    {
    }

    Window win;
    Application app;
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


    DISTRHO_DECLARE_NON_COPY_CLASS(ZynAddSubFXUI)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create UI, entry point */

START_NAMESPACE_DISTRHO

UI* createUI()
{
    const char* const bundlePath = 0;

    return new ZynAddSubFXUI(0, bundlePath);
}

END_NAMESPACE_DISTRHO
