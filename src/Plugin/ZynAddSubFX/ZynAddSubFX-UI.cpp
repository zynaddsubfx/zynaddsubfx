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
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

// DPF includes
#include "DistrhoUI.hpp"
#include "src/DistrhoPluginChecks.h"

// Custom includes
#include <cerrno>
#include <sys/wait.h>
#include <unistd.h>

/* ------------------------------------------------------------------------------------------------------------
 * External UI class */

// NOTE: The following code is non-portable!
//       It's only meant to be used in linux

class ExternalUI
{
public:
    ExternalUI(const intptr_t w)
        : pid(0),
          winId(w) {}

    ~ExternalUI()
    {
        terminateAndWaitForProcess();
    }

    void respawnAtURL(const int url)
    {
        terminateAndWaitForProcess();

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

        pid = vfork();

        switch (pid)
        {
        case 0:
            execvp(args[0], (char**)args);
            _exit(1);
            break;

        case -1:
            printf("Could not start external ui\n");
            break;
        }
    }

private:
    pid_t pid;
    const intptr_t winId;

    void terminateAndWaitForProcess()
    {
        if (pid <= 0)
            return;

        printf("Waiting for previous process to stop,,,\n");

        bool sendTerm = true;

        for (pid_t p;;)
        {
            p = ::waitpid(pid, nullptr, WNOHANG);

            switch (p)
            {
            case 0:
                if (sendTerm)
                {
                    sendTerm = false;
                    ::kill(pid, SIGTERM);
                }
                break;

            case -1:
                if (errno == ECHILD)
                {
                    printf("Done! (no such process)\n");
                    pid = 0;
                    return;
                }
                break;

            default:
                if (p == pid)
                {
                    printf("Done! (clean wait)\n");
                    pid = 0;
                    return;
                }
                break;
            }

            // 5 msec
            usleep(5*1000);
        }
    }
};

/* ------------------------------------------------------------------------------------------------------------
 * ZynAddSubFX UI class */

class ZynAddSubFXUI : public UI
{
public:
    ZynAddSubFXUI(const intptr_t winId)
        : UI(),
          externalUI(winId),
          oscPort(0)
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
                externalUI.respawnAtURL(port);
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
    ExternalUI externalUI;
    int oscPort;

    DISTRHO_DECLARE_NON_COPY_CLASS(ZynAddSubFXUI)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create plugin, entry point */

START_NAMESPACE_DISTRHO

UI* createUI(const intptr_t winId)
{
    return new ZynAddSubFXUI(winId);
}

END_NAMESPACE_DISTRHO

#ifdef DISTRHO_PLUGIN_TARGET_LV2
#include "src/DistrhoUILV2.cpp"
#endif
