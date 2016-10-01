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
#include <dlfcn.h>

typedef void *zest_t;
struct zest_handles {
    zest_t *(*zest_open)(const char *);
    void (*zest_close)(zest_t*);
    void (*zest_setup)(zest_t*);
    void (*zest_draw)(zest_t*);
    void (*zest_motion)(zest_t*, int x, int y);
    void (*zest_scroll)(zest_t*, int x, int y, int dx, int dy);
    void (*zest_mouse)(zest_t *z, int button, int action, int x, int y);
    void (*zest_key)(zest_t *z, char *key, bool press);
    void (*zest_resize)();
    void (*zest_special)(zest_t *z, int key, int press);
    int (*zest_tick)(zest_t*);
    zest_t *zest;
};

/* ------------------------------------------------------------------------------------------------------------
 * ZynAddSubFX UI class */

class ZynAddSubFXUI : public UI
{
public:
    ZynAddSubFXUI()
        : UI(1181, 659)
    {
        printf("[INFO] Opened the zynaddsubfx UI...\n");
        handle = dlopen("/opt/zyn-fusion/libzest.so", RTLD_LAZY);
        if(!handle) {
            printf("[ERROR] Cannot Open libzest.so\n");
            printf("[ERROR] '%s'\n", dlerror());
        }
        memset(&z, 0, sizeof(z));
#define get_sym(x) z.zest_##x = (decltype(z.zest_##x))dlsym(handle, "zest_"#x)
        get_sym(open);
        get_sym(setup);
        get_sym(close);
        get_sym(draw);
        get_sym(tick);
        get_sym(key);
        get_sym(motion);
        get_sym(scroll);
        get_sym(mouse);
        get_sym(special);
        printf("[INFO] Ready to run\n");
    }

    ~ZynAddSubFXUI() override
    {
        printf("[INFO:Zyn] zest_close()\n");
        if(z.zest)
            z.zest_close(z.zest);
        dlclose(handle);
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

   /* --------------------------------------------------------------------------------------------------------
    * UI Callbacks */

    bool onScroll(const ScrollEvent &ev) override
    {
        if(z.zest)
            z.zest_scroll(z.zest, ev.pos.getX(), ev.pos.getY(), ev.delta.getX(), ev.delta.getY());
        return false;
    }

    bool onSpecial(const SpecialEvent &ev) override
    {
        printf("special event = %d, %d\n", ev.key, ev.press);
        if(z.zest)
            z.zest_special(z.zest, ev.key, ev.press);
        return false;
    }

    bool onMouse(const MouseEvent &m) override
    {
        if(z.zest)
            z.zest_mouse(z.zest, m.button, m.press, m.pos.getX(), m.pos.getY());
        return false;
    }

    bool onMotion(const MotionEvent &m) override
    {
        if(z.zest)
            z.zest_motion(z.zest, m.pos.getX(), m.pos.getY());
        return false;
    }

   /**
      A function called to draw the view contents with OpenGL.
    */
    void onDisplay() override
    {
        if(!z.zest) {
            if(!z.zest_open)
                return;
            printf("[INFO:Zyn] zest_open()\n");
            char address[1024];
            snprintf(address, sizeof(address), "osc.udp://127.0.0.1:%d",oscPort);

            z.zest = z.zest_open(address);
            printf("[INFO:Zyn] zest_setup()\n");
            z.zest_setup(z.zest);
        }

        z.zest_draw(z.zest);
        repaint();
    }

    bool onKeyboard(const KeyboardEvent &ev)
    {
        char c[2] = {0};
        if(ev.key < 128)
            c[0] = ev.key;
        if(z.zest && c[0])
            z.zest_key(z.zest, c, ev.press);
        return true;
    }

    void uiIdle(void) override
    {
        if(z.zest)
            z.zest_tick(z.zest);
    }

    void uiReshape(uint width, uint height)
    {
    }

private:
    int oscPort;
    zest_handles z;
    void *handle;


    DISTRHO_DECLARE_NON_COPY_CLASS(ZynAddSubFXUI)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create UI, entry point */

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new ZynAddSubFXUI();
}

END_NAMESPACE_DISTRHO
