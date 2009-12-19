#ifndef NIOUI_H
#define NIOUT_H

#include <FL/Fl.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Pack.H>
#include <FL/Enumerations.H>

class NioUI : public Fl_Window
{
    public:
        NioUI();
        void refresh();

    private:
        const char *nullc, *alsac, *ossc, *jackc;

        static void nioToggle(Fl_Widget *w, void *name);
        Fl_Group  **groups;
        Fl_Button **buttons;
};
#endif

