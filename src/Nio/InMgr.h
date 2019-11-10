/*
  ZynAddSubFX - a software synthesizer

  InMgr.h - MIDI Input Manager
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#ifndef INMGR_H
#define INMGR_H

#include <string>
#include "ZynSema.h"
#include "SafeQueue.h"

namespace zyn {

enum midi_type {
    M_NOTE = 1, // for note
    M_CONTROLLER = 2, // for controller
    M_PGMCHANGE  = 3, // for program change
    M_PRESSURE   = 4, // for polyphonic aftertouch
    M_FLOAT_NOTE = 5  // for floating point note
};

struct MidiEvent {
    MidiEvent();
    int channel; //the midi channel for the event
    int type;    //type=1 for note, type=2 for controller
    int num;     //note, controller or program number
    int value;   //velocity or controller value
    int time;    //time offset of event (used only in jack->jack case at the moment)
    float log2_freq;	//type=5 for logarithmic representation of note
};

//super simple class to manage the inputs
class InMgr
{
    public:
        static InMgr &getInstance();
        ~InMgr();

        void putEvent(MidiEvent ev);

        /**Flush the Midi Queue*/
        void flush(unsigned frameStart, unsigned frameStop);

        bool empty() const;

        bool setSource(std::string name);

        std::string getSource() const;

        void setMaster(class Master *master);

        friend class EngineMgr;
    private:
        InMgr();
        class MidiIn *getIn(std::string name);
        SafeQueue<MidiEvent> queue;
        mutable ZynSema work;
        class MidiIn * current;

        /**the link to the rest of zyn*/
        class Master *master;
};

}

#endif
