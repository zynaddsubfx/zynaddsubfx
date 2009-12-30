#include <pthread.h>
#include "InMgr.h"
#include <iostream>

using namespace std;

InMgr *sysIn;

InMgr::InMgr(Master *_master)
    :master(_master)
{}

void InMgr::putEvent(MidiNote note)
{
    cout << note << endl;
    pthread_mutex_lock(&master->mutex);
    {
        dump.dumpnote(note.channel, note.note, note.velocity);

        if(note.velocity)
            master->noteOn(note.channel, note.note, note.velocity);
        else
            master->noteOff(note.channel, note.note);
    }
    pthread_mutex_unlock(&master->mutex);
}

void InMgr::putEvent(MidiCtl control)
{
    cout << control << endl;
    pthread_mutex_lock(&master->mutex);
    {
        dump.dumpcontroller(control.channel, control.controller,
                control.value);

        master->setController(control.channel, control.controller, control.value);
    }
    pthread_mutex_unlock(&master->mutex);
}


