#include "Control.h"

#ifndef DELAYCTL_H
#define DELAYCTL_H
/**A Control for Delays
 * 
 * Will vary from 0 seconds to 1.5 seconds*/
class DelayCtl:public Control
{
    public:
        DelayCtl();
        ~DelayCtl(){};
        std::string getString() const;
        void setmVal(char nval);
        char getmVal() const;
        float getiVal() const;
    private:
        float value;
};

#endif

