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
        void init(std::string, char type = 'f');

        //Refetch parameter information
        void update(void);
        void callback(Fl_Callback *cb, void *p = NULL);

        void cb(void);
        static void _cb(Fl_Widget *w, void *);
    private:
        string label_str;
        std::string full_path;
        std::string path;
        double real_value;
        char osc_type;
        std::pair<Fl_Callback*, void*> cb_data;
};
