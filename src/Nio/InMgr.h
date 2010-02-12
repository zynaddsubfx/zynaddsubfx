#ifndef INMGR_H
#define INMGR_H

#include <string>
#include <pthread.h>
#include <semaphore.h>
#include "../Misc/Atomic.h"
#include "SafeQueue.h"

enum midi_type{
    M_NOTE = 1,
    M_CONTROLLER = 2
};    //type=1 for note, type=2 for controller

struct MidiEvent
{
    MidiEvent();
    int channel; //the midi channel for the event
    int type;    //type=1 for note, type=2 for controller
    int num;     //note or contoller number
    int value;   //velocity or controller value
};

class Master;
//super simple class to manage the inputs
class InMgr
{
    public:
        InMgr(Master *nmaster);
        ~InMgr();

        void putEvent(MidiEvent ev);

        //run the InMgr
        void run();

        void *inputThread();

        bool setSource(std::string name);

        std::string getSource() const;

    private:
        SafeQueue<MidiEvent> queue;
        sem_t work;
        Atomic<bool> enabled;
        pthread_t inThread;

        /**the link to the rest of zyn*/
        Master *master;
};

extern InMgr *sysIn;
#endif

