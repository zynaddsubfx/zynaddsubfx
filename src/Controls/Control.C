#include "Control.h"

Control::Control(char ndefaultval)
  :defaultval(ndefaultval),lockqueue(-1),locked(false)
{}

void Control::lock()
{
    locked=true;
    lockqueue=-1;
}

void Control::ulock()
{
    if(locked&&lockqueue>=0)
        setmVal(lockqueue);
    locked=false;
}

