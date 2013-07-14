#pragma once
#include <FL/Fl_Slider.H>
#include "Fl_Osc_Widget.H"
#include <string>

using std::string; //yes this is bad form FIXME

class Fl_Osc_Slider:public Fl_Slider, Fl_Osc_Widget
{

    public:
        Fl_Osc_Slider(int X, int Y, int W, int H, const char *label = NULL);
       // string name,
        //        const char *metadata);

        virtual ~Fl_Osc_Slider(void);
        void OSC_value(float);
        void init(Fl_Osc_Interface *, std::string);
        void init(std::string);

        void cb(void);
        static void _cb(Fl_Widget *w, void *);
    private:
        string label_str;
        std::string full_path;
        double real_value;
        class Fl_Osc_Interface *osc;
};
