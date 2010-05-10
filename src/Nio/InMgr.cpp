#include "InMgr.h"
#include "MidiIn.h"
#include "EngineMgr.h"
#include "../Misc/Master.h"
#include <iostream>

using namespace std;

InMgr *sysIn;

ostream &operator<<(ostream &out, const MidiEvent& ev)
{
    if(ev.type == M_NOTE)
        out << "MidiNote: note("     << ev.num      << ")\n"
            << "          channel("  << ev.channel  << ")\n"
            << "          velocity(" << ev.value    << ")";
    else
        out << "MidiCtl: controller(" << ev.num     << ")\n"
            << "         channel("    << ev.channel << ")\n"
            << "         value("      << ev.value   << ")";
    return out;
}

MidiEvent::MidiEvent()
    :channel(0),type(0),num(0),value(0)
{}

InMgr::InMgr(Master *_master)
    :queue(100), master(_master)
{
    current = NULL;
    sem_init(&work, PTHREAD_PROCESS_PRIVATE, 0);
}

InMgr::~InMgr()
{
    //lets stop the consumer thread
    sem_destroy(&work);
}

void InMgr::putEvent(MidiEvent ev)
{
    if(queue.push(ev)) //check for error
        cerr << "ERROR: Midi Ringbuffer is FULL" << endl;
    else
        sem_post(&work);
}

void InMgr::flush()
{
    MidiEvent ev;
    while(!sem_trywait(&work)) {
        queue.pop(ev);
        cout << ev << endl;

        if(M_NOTE == ev.type) {
            dump.dumpnote(ev.channel, ev.num, ev.value);

            if(ev.value)
                master->noteOn(ev.channel, ev.num, ev.value);
            else
                master->noteOff(ev.channel, ev.num);
        }
        else {
            dump.dumpcontroller(ev.channel, ev.num, ev.value);
            master->setController(ev.channel, ev.num, ev.value);
        }
    }
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
        (current = dynamic_cast<MidiIn *>(sysEngine->getEng("NULL")))->setMidiEn(true);

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
    return dynamic_cast<MidiIn *>(sysEngine->getEng(name));
}

