#include "InMgr.h"
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
    :queue(100), enabled(false), master(_master)
{
    sem_init(&work, PTHREAD_PROCESS_PRIVATE, 0);
}

InMgr::~InMgr()
{
    //lets stop the consumer thread
    enabled = false;
    sem_post(&work);
    pthread_join(inThread, NULL);

    sem_destroy(&work);
}

void InMgr::putEvent(MidiEvent ev)
{
    if(queue.push(ev)) //check for error
        cout << "Error, Midi Ringbuffer is FULL" << endl;
    else
        sem_post(&work);
}

void *_inputThread(void *arg)
{
    return static_cast<InMgr *>(arg)->inputThread();
}

void InMgr::run()
{
    enabled = true;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&inThread, &attr, _inputThread, this);
}

void *InMgr::inputThread()
{
    MidiEvent ev;
    while(enabled()) {
        sem_wait(&work);
        queue.pop(ev);
        cout << ev << endl;

        pthread_mutex_lock(&master->mutex);
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
        pthread_mutex_unlock(&master->mutex);
    }
    return NULL;
}

