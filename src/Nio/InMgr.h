#ifndef INMGR_H
#define INMGR_H

#include <string>
#include "MidiEvent.h"
#include "../Misc/Master.h"


class Master;
//super simple class to manage the inputs
class InMgr
{
    public:
        InMgr(Master *nmaster);
        ~InMgr(){};

        //only interested in Notes and Controllers for now
        void putEvent(MidiNote note);
        void putEvent(MidiCtl control);

    private:
        /**the link to the rest of zyn*/
        Master *master;
};

extern InMgr *sysIn;
#endif

