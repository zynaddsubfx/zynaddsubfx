/*
  ZynAddSubFX - a software synthesizer

  ZynAddSubFX.cpp - DPF + ZynAddSubFX Plugin
  Copyright (C) 2015-2016 Filipe Coelho
  Author: Filipe Coelho

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

// DPF includes
#include "DistrhoPlugin.hpp"

// ZynAddSubFX includes
#include "zyn-version.h"
#include "Misc/Master.h"
#include "Misc/MiddleWare.h"
#include "Misc/Part.h"
#include "Misc/Util.h"

// Extra includes
#include "extra/Mutex.hpp"
#include "extra/Thread.hpp"
#include "extra/ScopedPointer.hpp"

#include <lo/lo.h>

/* ------------------------------------------------------------------------------------------------------------
 * MiddleWare thread class */

class MiddleWareThread : public Thread
{
public:
      class ScopedStopper
      {
      public:
          ScopedStopper(MiddleWareThread& mwt) noexcept
              : wasRunning(mwt.isThreadRunning()),
                thread(mwt),
                middleware(mwt.middleware)
          {
              if (wasRunning)
                  thread.stop();
          }

          ~ScopedStopper() noexcept
          {
              if (wasRunning)
                  thread.start(middleware);
          }

          void updateMiddleWare(zyn::MiddleWare* const mw) noexcept
          {
              middleware = mw;
          }

      private:
          const bool wasRunning;
          MiddleWareThread& thread;
          zyn::MiddleWare* middleware;

          DISTRHO_PREVENT_HEAP_ALLOCATION
          DISTRHO_DECLARE_NON_COPY_CLASS(ScopedStopper)
      };

      MiddleWareThread()
          : Thread("ZynMiddleWare"),
            middleware(nullptr) {}

      void start(zyn::MiddleWare* const mw) noexcept
      {
          middleware = mw;
          startThread();
      }

      void stop() noexcept
      {
          stopThread(1000);
          middleware = nullptr;
      }

protected:
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

private:
    zyn::MiddleWare* middleware;

    DISTRHO_DECLARE_NON_COPY_CLASS(MiddleWareThread)
};

/* ------------------------------------------------------------------------------------------------------------
 * ZynAddSubFX plugin class */

class ZynAddSubFX : public Plugin
{
public:
    ZynAddSubFX()
        : Plugin(kParamCount, 1, 1), // 1 program, 1 state
          master(nullptr),
          middleware(nullptr),
          defaultState(nullptr),
          oscPort(0),
          middlewareThread(new MiddleWareThread())
    {
        synth.buffersize = static_cast<int>(getBufferSize());
        synth.samplerate = static_cast<uint>(getSampleRate());

        if (synth.buffersize > 32)
            synth.buffersize = 32;

        synth.alias();

        _initMaster();

        defaultState = _getState();

        middlewareThread->start(middleware);
    }

    ~ZynAddSubFX() override
    {
        middlewareThread->stop();
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
        return "Synthesizer featuring additive, subtractive, and Fourier "
               "synthesis methods, a variety of modulators, powerful "
               "oscillator editors, and a variety of built-in effects.";
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
        return d_version(zyn::version.get_major(),
                         zyn::version.get_minor(),
                         zyn::version.get_revision());
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

   /**
      Initialize the parameter @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initParameter(uint32_t index, Parameter& parameter) noexcept override
    {
        switch (index)
        {
        case kParamOscPort:
            parameter.hints  = kParameterIsOutput;
            parameter.name   = "OSC Port";
            parameter.symbol = "osc_port";
            parameter.unit   = "";
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 999999.0f;
            parameter.ranges.def = 0.0f;
            break;
        }
        if(index <= kParamSlot16) {
            parameter.hints  = kParameterIsAutomable;
            parameter.name   = ("Slot " + zyn::to_s(index-kParamSlot1 + 1)).c_str();
            parameter.symbol = ("slot"  + zyn::to_s(index-kParamSlot1 + 1)).c_str();
            parameter.unit   = "";
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            parameter.ranges.def = 0.5f;
        }
    }

   /**
      Get the current value of a parameter.
      The host may call this function from any context, including realtime processing.
    */
    float getParameterValue(uint32_t index) const noexcept override
    {
        switch (index)
        {
        case kParamOscPort:
            return oscPort;
        }
        if(index <= kParamSlot16) {
            return master->automate.getSlot(index - kParamSlot1);
        }
        return 0.0f;
    }

   /**
      Change a parameter value.
      The host may call this function from any context, including realtime processing.
      When a parameter is marked as automable, you must ensure no non-realtime operations are performed.
      @note This function will only be called for parameter inputs.
    */
    void setParameterValue(uint32_t index, float value) noexcept override
    {
        if(index <= kParamSlot16)
            master->automate.setSlot(index - kParamSlot1, value);
    }

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
        const MiddleWareThread::ScopedStopper mwss(*middlewareThread);
        const MutexLocker cml(mutex);

        if(key && strlen(key) > 1000 && (!value || strlen(value) < 1000)) {
            //Loading a Jackoo VST File
            value = key;
        }

        master->defaults();
        master->putalldata(value);
        master->applyparameters();
        master->initialize_rt();

        middleware->updateResources(master);
    }

   /* --------------------------------------------------------------------------------------------------------
    * Audio/MIDI Processing */

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
                std::memset(outputs[0], 0, sizeof(float)*frames);
                std::memset(outputs[1], 0, sizeof(float)*frames);
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

            case 0xC0: {
                const int program = midiEvent.data[1];

                for(int i=0; i < NUM_MIDI_PARTS; ++i) {
                    //set the program of the parts assigned to the midi channel
                    if(master->part[i]->Prcvchn == channel) {
                        middleware->pendingSetProgram(i, program);
                    }
                }
            } break;

            case 0xE0: {
                const uint8_t lsb = midiEvent.data[1];
                const uint8_t msb = midiEvent.data[2];
                const int   value = ((msb << 7) | lsb) - 8192;

                master->setController(channel, zyn::C_pitchwheel, value);
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
        MiddleWareThread::ScopedStopper mwss(*middlewareThread);

        char* const state(_getState());

        _deleteMaster();

        synth.buffersize = static_cast<int>(newBufferSize);

        if (synth.buffersize > 32)
            synth.buffersize = 32;

        synth.alias();

        _initMaster();
        mwss.updateMiddleWare(middleware);

        setState(nullptr, state);
        std::free(state);
    }

   /**
      Callback to inform the plugin about a sample rate change.
      This function will only be called when the plugin is deactivated.
    */
    void sampleRateChanged(double newSampleRate) override
    {
        MiddleWareThread::ScopedStopper mwss(*middlewareThread);

        char* const state(_getState());

        _deleteMaster();

        synth.samplerate = static_cast<uint>(newSampleRate);
        synth.alias();

        _initMaster();
        mwss.updateMiddleWare(middleware);

        setState(nullptr, state);
        std::free(state);
    }

private:
    zyn::Config      config;
    zyn::Master*     master;
    zyn::MiddleWare* middleware;
    zyn::SYNTH_T     synth;

    Mutex mutex;
    char* defaultState;
    int   oscPort;

    ScopedPointer<MiddleWareThread> middlewareThread;

    char* _getState() const
    {
        const MiddleWareThread::ScopedStopper mwss(*middlewareThread);

        char* data = nullptr;
        master->getalldata(&data);
        return data;
    }

    void _initMaster()
    {
        middleware = new zyn::MiddleWare(std::move(synth), &config);
        middleware->setUiCallback(__uiCallback, this);
        middleware->setIdleCallback(__idleCallback, this);
        _masterChangedCallback(middleware->spawnMaster());

        if (char* portStr = middleware->getServerPort())
        {
            oscPort = std::atoi(portStr);
            std::free(portStr);
        }
        else
        {
            oscPort = 0;
        }
    }

    void _deleteMaster()
    {
        master = nullptr;
        delete middleware;
        middleware = nullptr;
    }

   /* --------------------------------------------------------------------------------------------------------
    * ZynAddSubFX Callbacks */

    void _masterChangedCallback(zyn::Master* m)
    {
        master = m;
        master->setMasterChangedCallback(__masterChangedCallback, this);
    }

    static void __masterChangedCallback(void* ptr, zyn::Master* m)
    {
        ((ZynAddSubFX*)ptr)->_masterChangedCallback(m);
    }

   /* -------------------------------------------------------------------------------------------------------- */

    void _uiCallback(const char* const)
    {
        // this can be used to receive messages from UI
        // to be handled soon for parameters
    }

    static void __uiCallback(void* ptr, const char* msg)
    {
        ((ZynAddSubFX*)ptr)->_uiCallback(msg);
    }

   /* -------------------------------------------------------------------------------------------------------- */

    void _idleCallback()
    {
        // this can be used to give host idle time
        // for now LV2 doesn't support this, and only some VST hosts support it
    }

    static void __idleCallback(void* ptr)
    {
        ((ZynAddSubFX*)ptr)->_idleCallback();
    }

    DISTRHO_DECLARE_NON_COPY_CLASS(ZynAddSubFX)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create plugin, entry point */

START_NAMESPACE_DISTRHO

Plugin* createPlugin()
{
    zyn::isPlugin = true;
    return new ZynAddSubFX();
}

END_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * Dummy variables and functions for linking purposes */

namespace zyn {
class WavFile;
namespace Nio {
   void masterSwap(zyn::Master*){}
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
}
