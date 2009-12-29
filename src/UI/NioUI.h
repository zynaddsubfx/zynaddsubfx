#ifndef NIOUI_H
#define NIOUT_H

#include <FL/Fl.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Spinner.H>
#include <FL/Enumerations.H>
#include <list>
#include <string>

struct NioTab : public Fl_Group
{
    NioTab(std::string name);

    void refresh();
    static void nioToggle(Fl_Widget *w, void *arg);
    static void nioBuffer(Fl_Widget *w, void *arg);

    Fl_Light_Button outEnable;
    Fl_Spinner buffer;
    const std::string name;
};

class NioUI : public Fl_Window
{
    public:
        NioUI();
        void refresh();
    private:
        std::list<NioTab *> tabs;
};

#endif

