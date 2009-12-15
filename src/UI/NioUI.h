#ifndef NIOUI_H
#define NIOUT_H

#include <FL/Fl.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Pack.H>
#include <FL/Enumerations.H>

class Pack : public Fl_Pack
{
    public:
        Pack(int x, int y, int w, int h);
    private:
        Fl_Light_Button b1,b2,b3;
        static void nioToggle(Fl_Widget *w, void *name);
};

class NioUI : public Fl_Window
{
    public:
        NioUI();
    private:
        Pack foo;
};
#endif

