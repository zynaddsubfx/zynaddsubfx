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

//removed from code for now
#if 0
struct NioTab : public Fl_Group
{
    NioTab(std::string name, bool _midi, bool _audio);

    void refresh();
    static void nioToggle(Fl_Widget *w, void *arg);
    static void audioToggle(Fl_Widget *w, void *arg);
    static void midiToggle(Fl_Widget *w, void *arg);
    static void nioBuffer(Fl_Widget *w, void *arg);

    Fl_Light_Button enable;
    Fl_Light_Button *midi;
    Fl_Light_Button *audio;
    Fl_Spinner *buffer;
    const std::string name;
};
#endif


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

