/*
  ZynAddSubFX - a software synthesizer

  ZynAddSubFX.cpp - DPF + ZynAddSubFX Plugin
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
#include "DistrhoPlugin.hpp"

// ZynAddSubFX includes
#include "Misc/Master.h"
#include "Misc/MiddleWare.h"
#include "Misc/Part.h"
#include "Misc/Util.h"

// Extra includes
#include "extra/Mutex.hpp"
#include "extra/Thread.hpp"

/* ------------------------------------------------------------------------------------------------------------
 * ZynAddSubFX plugin class */

class ZynAddSubFX : public Plugin,
                    private Thread
{
public:
    ZynAddSubFX()
        : Plugin(0, 1, 1), // 0 parameters, 1 program, 1 state
          Thread("ZynMwTick"),
          master(nullptr),
          middleware(nullptr),
          active(false),
          defaultState(nullptr)
    {
        config.init();

        synth.buffersize = static_cast<int>(getBufferSize());
        synth.samplerate = static_cast<uint>(getSampleRate());

        if (synth.buffersize > 32)
            synth.buffersize = 32;

        synth.alias();

        _initMaster();

        defaultState = _getState();
    }

    ~ZynAddSubFX() override
    {
        _deleteMaster();
        std::free(defaultState);
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
        return "ZynAddSubFX";
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
        return d_version(2, 5, 3);
    }

   /**
      Get the plugin unique Id.
      This value is used by LADSPA, DSSI and VST plugin formats.
    */
    int64_t getUniqueId() const noexcept override
    {
        return d_cconst('Z', 'A', 'S', 'F');
    }

   /* --------------------------------------------------------------------------------------------------------
    * Parameters, empty for now */

    void  initParameter(uint32_t, Parameter&) noexcept override {}
    float getParameterValue(uint32_t) const noexcept override { return 0.0f; }
    void  setParameterValue(uint32_t index, float value) noexcept override {}

   /* --------------------------------------------------------------------------------------------------------
    * Programs */

   /**
      Set the name of the program @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initProgramName(uint32_t index, String& programName) override
    {
        programName = "Default";
    }

   /**
      Load a program.
      The host may call this function from any context, including realtime processing.
    */
    void loadProgram(uint32_t index) override
    {
        setState(nullptr, defaultState);
    }

   /* --------------------------------------------------------------------------------------------------------
    * States */

   /**
      Set the state key and default value of @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initState(uint32_t, String& stateKey, String& defaultStateValue) override
    {
        stateKey = "state";
        defaultStateValue = defaultState;
    }

   /**
      Get the value of an internal state.
      The host may call this function from any non-realtime context.
    */
    String getState(const char*) const override
    {
        return String(_getState(), false);
    }

   /**
      Change an internal state @a key to @a value.
    */
    void setState(const char* key, const char* value) override
    {
        const MutexLocker cml(mutex);

        master->putalldata(value);
        master->applyparameters();
        master->initialize_rt();

        middleware->updateResources(master);
    }

   /* --------------------------------------------------------------------------------------------------------
    * Audio/MIDI Processing */

   /**
      Activate this plugin.
    */
    void activate() noexcept override
    {
        active = true;
    }

   /**
      Deactivate this plugin.
    */
    void deactivate() noexcept override
    {
        active = false;
    }

   /**
      Run/process function for plugins with MIDI input.
      @note Some parameters might be null if there are no audio inputs/outputs or MIDI events.
    */
    void run(const float**, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        if (! mutex.tryLock())
        {
            //if (! isOffline())
            {
                std::memset(outputs[0], 0, frames);
                std::memset(outputs[1], 0, frames);
                return;
            }

            mutex.lock();
        }

        uint32_t framesOffset = 0;

        for (uint32_t i=0; i<midiEventCount; ++i)
        {
            const MidiEvent& midiEvent(midiEvents[i]);

            if (midiEvent.frame >= frames)
                continue;
            if (midiEvent.size > MidiEvent::kDataSize)
                continue;
            if (midiEvent.data[0] < 0x80 || midiEvent.data[0] >= 0xF0)
                continue;

            if (midiEvent.frame > framesOffset)
            {
                master->GetAudioOutSamples(midiEvent.frame-framesOffset, synth.samplerate, outputs[0]+framesOffset,
                                                                                           outputs[1]+framesOffset);
                framesOffset = midiEvent.frame;
            }

            const uint8_t status  = midiEvent.data[0] & 0xF0;
            const char    channel = midiEvent.data[0] & 0x0F;

            switch (status)
            {
            case 0x80: {
                const char note = static_cast<char>(midiEvent.data[1]);

                master->noteOff(channel, note);
            } break;

            case 0x90: {
                const char note = static_cast<char>(midiEvent.data[1]);
                const char velo = static_cast<char>(midiEvent.data[2]);

                master->noteOn(channel, note, velo);
            } break;

            case 0xA0: {
                const char note     = static_cast<char>(midiEvent.data[1]);
                const char pressure = static_cast<char>(midiEvent.data[2]);

                master->polyphonicAftertouch(channel, note, pressure);
            } break;

            case 0xB0: {
                const int control = midiEvent.data[1];
                const int value   = midiEvent.data[2];

                // skip controls which we map to parameters
                //if (getIndexFromZynControl(midiEvent.data[1]) != kParamCount)
                //    continue;

                master->setController(channel, control, value);
            } break;

            case 0xE0: {
                const uint8_t lsb = midiEvent.data[1];
                const uint8_t msb = midiEvent.data[2];
                const int   value = ((msb << 7) | lsb) - 8192;

                master->setController(channel, C_pitchwheel, value);
            } break;
            }
        }

        if (frames > framesOffset)
            master->GetAudioOutSamples(frames-framesOffset, synth.samplerate, outputs[0]+framesOffset,
                                                                              outputs[1]+framesOffset);

        mutex.unlock();
    }

   /* --------------------------------------------------------------------------------------------------------
    * Callbacks */

   /**
      Callback to inform the plugin about a buffer size change.
      This function will only be called when the plugin is deactivated.
      @note This value is only a hint!
            Hosts might call run() with a higher or lower number of frames.
    */
    void bufferSizeChanged(uint32_t newBufferSize) override
    {
        char* const state(_getState());

        _deleteMaster();

        synth.buffersize = static_cast<int>(newBufferSize);

        if (synth.buffersize > 32)
            synth.buffersize = 32;

        synth.alias();

        _initMaster();

        setState(nullptr, state);
        std::free(state);
    }

   /**
      Callback to inform the plugin about a sample rate change.
      This function will only be called when the plugin is deactivated.
    */
    void sampleRateChanged(double newSampleRate) override
    {
        char* const state(_getState());

        _deleteMaster();

        synth.samplerate = static_cast<uint>(newSampleRate);
        synth.alias();

        _initMaster();

        setState(nullptr, state);
        std::free(state);
    }

private:
    Config      config;
    Master*     master;
    MiddleWare* middleware;
    SYNTH_T     synth;

    bool  active;
    Mutex mutex;
    char* defaultState;

    char* _getState() const
    {
        char* data = nullptr;

        if (active)
        {
            middleware->doReadOnlyOp([this, &data]{
                master->getalldata(&data);
            });
        }
        else
        {
            master->getalldata(&data);
        }

        return data;
    }

    void _initMaster()
    {
        middleware = new MiddleWare(std::move(synth), &config);
        middleware->setUiCallback(__uiCallback, this);
        middleware->setIdleCallback(__idleCallback, this);
        _masterChangedCallback(middleware->spawnMaster());
        startThread();
    }

    void _deleteMaster()
    {
        stopThread(1000);

        master = nullptr;
        delete middleware;
        middleware = nullptr;
    }

    void _masterChangedCallback(Master* m)
    {
        master = m;
        master->setMasterChangedCallback(__masterChangedCallback, this);
    }

    static void __masterChangedCallback(void* ptr, Master* m)
    {
        ((ZynAddSubFX*)ptr)->_masterChangedCallback(m);
    }

    void _uiCallback(const char* const)
    {
    }

    static void __uiCallback(void* ptr, const char* msg)
    {
        ((ZynAddSubFX*)ptr)->_uiCallback(msg);
    }

    void _idleCallback()
    {
        // this can be used to give host idle time
        // for now LV2 doesn't support this, and only some VST hosts support it
    }

    static void __idleCallback(void* ptr)
    {
        ((ZynAddSubFX*)ptr)->_idleCallback();
    }

    void run() noexcept override
    {
        for (; ! shouldThreadExit();)
        {
            try {
                middleware->tick();
            } catch(...) {}

            d_msleep(1);
        }
    }

    DISTRHO_DECLARE_NON_COPY_CLASS(ZynAddSubFX)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create plugin, entry point */

START_NAMESPACE_DISTRHO

Plugin* createPlugin()
{
    return new ZynAddSubFX();
}

END_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * Dummy variables and functions for linking purposes */

class WavFile;
namespace Nio {
   void masterSwap(Master*){}
   bool setSource(std::string){return true;}
   bool setSink(std::string){return true;}
   std::set<std::string> getSources(void){return std::set<std::string>();}
   std::set<std::string> getSinks(void){return std::set<std::string>();}
   std::string getSource(void){return "";}
   std::string getSink(void){return "";}
   void waveNew(WavFile*){}
   void waveStart(){}
   void waveStop(){}
}
