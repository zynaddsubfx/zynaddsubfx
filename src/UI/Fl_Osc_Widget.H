#pragma once
#include <string>
#include <cstring>
#include <cmath>
#include <FL/Fl_Group.H>

class Fl_Osc_Widget
{
    public:
        Fl_Osc_Widget(void) {}
        virtual ~Fl_Osc_Widget(void){};

        virtual void OSC_value(float) {}
        virtual void OSC_value(bool) {}
        virtual void OSC_value(int) {}
        virtual void OSC_value(char) {}
        virtual void OSC_value(unsigned,void*) {}

        std::string loc;
        class Fl_Osc_Interface *osc;
};
