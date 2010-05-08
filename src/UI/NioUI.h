#ifndef NIOUI_H
#define NIOUT_H

#include <FL/Fl.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Spinner.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Choice.H>
#include <list>
#include <string>

class NioUI : public Fl_Window
{
    public:
        NioUI();
        void refresh();
    private:
        //std::list<NioTab *> tabs;

        Fl_Choice *midi;
        Fl_Choice *audio;
        static void midiCallback(Fl_Widget *c);
        static void audioCallback(Fl_Widget *c);
};

#endif

