#pragma once
#include <FL/Fl_Counter.H>
#include "Fl_Osc_Widget.H"
#include <string>

class Fl_Osc_Counter: public Fl_Counter, public Fl_Osc_Widget
{
    public:
        Fl_Osc_Counter(int x, int y, int w, int h, const char *label=0);
        void init(const char *);
        void OSC_value(char);
        using Fl_Osc_Widget::OSC_value;
        
        //Refetch parameters
        void update(void);
        void callback(Fl_Callback *cb, void *p = NULL);

        void cb(void);
    private:
        std::string path;
        std::pair<Fl_Callback*, void*> cb_data;
};
