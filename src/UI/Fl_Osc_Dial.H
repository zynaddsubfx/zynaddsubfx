#pragma once
#include <FL/Fl_Dial.H>
#include "WidgetPDial.h"
#include "Fl_Osc_Widget.H"
#include <string>

class Fl_Osc_Dial:public WidgetPDial, Fl_Osc_Widget
{

    public:
        Fl_Osc_Dial(int X, int Y, int W, int H, const char *label = NULL);
        virtual ~Fl_Osc_Dial(void);
        void init(const char *path);
        //void OSC_value(float);
        void OSC_value(char);
        using Fl_Osc_Widget::OSC_value;

        //Refetch parameter information
        void update(void);
        void callback(Fl_Callback *cb, void *p = NULL);

        void cb(void);
    private:
        std::string full_path;
        class Fl_Osc_Interface *osc;
        std::pair<Fl_Callback*, void*> cb_data;
};
