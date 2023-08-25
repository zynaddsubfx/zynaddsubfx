/*
  ZynAddSubFX - a software synthesizer

  InMgr.cpp - MIDI Input Manager
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "InMgr.h"
#include "MidiIn.h"
#include "EngineMgr.h"
#include "../Misc/Master.h"
#include "../Misc/Part.h"
#include "../Misc/MiddleWare.h"
#include <rtosc/thread-link.h>
#include <iostream>
using namespace std;

extern zyn::MiddleWare *middleware;

namespace zyn {

ostream &operator<<(ostream &out, const MidiEvent &ev)
{
    switch(ev.type) {
        case M_NOTE:
            out << "MidiNote: note(" << ev.num << ")\n"
            << "          channel(" << ev.channel << ")\n"
            << "          velocity(" << ev.value << ")";
            break;

        case M_FLOAT_NOTE:
            out << "MidiNote: note(" << ev.num << ")\n"
            << "          channel(" << ev.channel << ")\n"
            << "          velocity(" << ev.value << ")\n"
            << "          log2_freq(" << ev.log2_freq << ")";
            break;

        case M_CONTROLLER:
            out << "MidiCtl: controller(" << ev.num << ")\n"
            << "         channel(" << ev.channel << ")\n"
            << "         value(" << ev.value << ")";
            break;

        case M_FLOAT_CTRL:
            out << "MidiNote: controller(" << ev.num << ")\n"
            << "          channel(" << ev.channel << ")\n"
            << "          note(" << ev.value << ")\n"
            << "          log2_value(" << ev.log2_freq << ")";
            break;

        case M_PGMCHANGE:
            out << "PgmChange: program(" << ev.num << ")\n"
            << "           channel(" << ev.channel << ")";
            break;
    }

    return out;
}

MidiEvent::MidiEvent()
    :channel(0), type(0), num(0), value(0), time(0)
{}

InMgr &InMgr::getInstance()
{
    static InMgr instance;
    return instance;
}

InMgr::InMgr()
    :queue(256), master(NULL)
{
    current = NULL;
    work.init(PTHREAD_PROCESS_PRIVATE, 0);
}

InMgr::~InMgr()
{
    //lets stop the consumer thread
}

void InMgr::putEvent(MidiEvent ev)
{
    if(queue.push(ev)) //check for error
        cerr << "ERROR: MIDI ringbuffer is FULL" << endl;
    else
        work.post();
}

bool InMgr::flush(unsigned frameStart, unsigned frameStop)
{
    MidiEvent ev;
    bool endReached = true;

    while(!work.trywait()) {
        queue.peak(ev);
        if(ev.time < (int)frameStart || ev.time > (int)frameStop) {
            //Check if end was reached
            endReached = ev.time < (int)frameStart;
            //Back out of transaction
            work.post();
            //printf("%d vs [%d..%d]\n",ev.time, frameStart, frameStop);
            break;
        }
        queue.pop(ev);
        //cout << ev << endl;

        switch(ev.type) {
            case M_NOTE:
                master->noteOn(ev.channel, ev.num, ev.value);
                break;

            case M_FLOAT_NOTE:
                master->noteOn(ev.channel, ev.num, ev.value, ev.log2_freq);
                break;

            case M_CONTROLLER:
                if(ev.num == C_bankselectmsb) {        // Change current bank
                    middleware->spawnMaster()->bToU->write("/forward", "");
                    middleware->spawnMaster()->bToU->write("/bank/msb", "i", ev.value);
                    middleware->spawnMaster()->bToU->write("/bank/bank_select", "i", ev.value);

                } else if(ev.num == C_bankselectlsb)  {// Change current bank (LSB)
                    middleware->spawnMaster()->bToU->write("/forward", "");
                    middleware->spawnMaster()->bToU->write("/bank/lsb", "i", ev.value);
                } else
                    master->setController(ev.channel, ev.num, ev.value);
                break;

            case M_FLOAT_CTRL:
                master->setController(ev.channel, ev.num, ev.value, ev.log2_freq);
                break;

            case M_PGMCHANGE:
                for(int i=0; i < NUM_MIDI_PARTS; ++i) {
                    //set the program of the parts assigned to the midi channel
                    if(master->part[i]->Prcvchn == ev.channel) {
                        middleware->pendingSetProgram(i, ev.num);
                    }
                }
                break;

            case M_PRESSURE:
                master->polyphonicAftertouch(ev.channel, ev.num, ev.value);
                break;
        }
    }
    return endReached;
}

bool InMgr::empty(void) const
{
    int semvalue = work.getvalue();
    return semvalue <= 0;
}

bool InMgr::setSource(string name)
{
    MidiIn *src = getIn(name);

    if(!src)
        return false;

    if(current)
        current->setMidiEn(false);
    current = src;
    current->setMidiEn(true);

    bool success = current->getMidiEn();

    //Keep system in a valid state (aka with a running driver)
    if(!success)
        (current = getIn("NULL"))->setMidiEn(true);

    return success;
}

string InMgr::getSource() const
{
    if(current)
        return current->name;
    else
        return "ERROR";
}

MidiIn *InMgr::getIn(string name)
{
    EngineMgr &eng = EngineMgr::getInstance(NULL);
    return dynamic_cast<MidiIn *>(eng.getEng(name));
}

void InMgr::setMaster(Master *master_)
{
    master = master_;
}

}
