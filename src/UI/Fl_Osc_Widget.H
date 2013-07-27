#pragma once
#include <string>
#include <cstring>
#include <cmath>
#include "Fl_Osc_Pane.H"
#include <FL/Fl_Group.H>

class Fl_Osc_Widget
{
    public:
        Fl_Osc_Widget(void) {}
        virtual ~Fl_Osc_Widget(void){};

        virtual void OSC_value(float) {}
        virtual void OSC_value(int) {}
        virtual void OSC_value(char) {}
        virtual void OSC_value(unsigned,void*) {}

        //labeled forwarding methods
        virtual void OSC_value(float x, const char *) {OSC_value(x);}
        virtual void OSC_value(bool x, const char *) {OSC_value(x);}
        virtual void OSC_value(char x, const char *) {OSC_value(x);}
        virtual void OSC_value(unsigned x, void *v, const char *)
        {OSC_value(x,v);}

        virtual void OSC_raw(const char *)
        {}

        Fl_Osc_Pane *fetch_osc_pane(Fl_Widget *w)
        {
            if(!w)
                return NULL;

            Fl_Osc_Pane *pane = dynamic_cast<Fl_Osc_Pane*>(w->parent());
            if(pane)
                return pane;
            return fetch_osc_pane(w->parent());
        }

        std::string loc;
        class Fl_Osc_Interface *osc;
};
