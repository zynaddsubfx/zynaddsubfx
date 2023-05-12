/*
  ZynAddSubFX - a software synthesizer

  MiddleWare.h - RT & Non-RT Glue Layer
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include <functional>
#include <cstdarg>
#include <string>

class Fl_Osc_Interface;

namespace rtosc {
    struct MergePorts;
}

namespace zyn {

struct SYNTH_T;
class  Master;
class PresetsStore;

//Link between realtime and non-realtime layers
class MiddleWare
{
    public:
        MiddleWare(SYNTH_T synth, class Config *config,
                   int preferred_port = -1);
        ~MiddleWare(void);
        void updateResources(Master *m);
        //returns internal master pointer
        class Master *spawnMaster(void);

        //Enable AutoSave Functionality
        void enableAutoSave(int interval_sec=60);

        //Check for old automatic saves which should only exist if multiple
        //instances are in use OR when there was a crash
        //
        //When an old save is found return the id of the save file
        int checkAutoSave(void) const;

        void removeAutoSave(void);

        //return  UI interface
        Fl_Osc_Interface *spawnUiApi(void);
        //Set callback to push UI events to
        void setUiCallback(void(*cb)(void*,const char *),void *ui);
        //Set callback to run while busy
        void setIdleCallback(void(*cb)(void*),void *ptr);
        //Handle events
        void tick(void);
        //Do A Readonly Operation (For Parameter Copy)
        void doReadOnlyOp(std::function<void()>);
        //Handle a rtosc Message uToB
        void transmitMsg(const char *);
        //Handle a rtosc Message uToB
        void transmitMsg(const char *, const char *args, ...);
        //Handle a rtosc Message uToB
        void transmitMsg_va(const char *, const char *args, va_list va);

        //Handle a rtosc Message uToB, if sender is GUI
        void transmitMsgGui(const char * msg);
        //Handle a rtosc Message uToB, if sender is GUI
        void transmitMsgGui(const char *, const char *args, ...);
        //Handle a rtosc Message uToB, if sender is GUI
        void transmitMsgGui_va(const char *, const char *args, va_list va);

        //Send a message to middleware from an arbitrary thread
        void messageAnywhere(const char *msg, const char *args, ...);

        //Indicate that a bank will be loaded
        //NOTE: Can only be called by realtime thread
        void pendingSetBank(int bank);

        //Indicate that a program will be loaded on a known part
        //NOTE: Can only be called by realtime thread
        void pendingSetProgram(int part, int program);

        std::string getProgramName(int program) const;

        //Get/Set the active bToU url
        std::string activeUrl(void) const;
        void activeUrl(std::string u);
        //View Synthesis Parameters
        const SYNTH_T &getSynth(void) const;
        //liblo stuff
        char* getServerAddress(void) const;
        char* getServerPort(void) const;

        const PresetsStore& getPresetsStore() const;
        PresetsStore& getPresetsStore();

        //!Make @p new_master the current master
        //!@warning use with care, and only in frozen state
        void switchMaster(Master* new_master);

        static const rtosc::MergePorts& getAllPorts();

    private:
        class MiddleWareImpl *impl;
};

}

