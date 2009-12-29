#ifndef NIOUI_H
#define NIOUT_H

#include <FL/Fl.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Pack.H>
#include <FL/Enumerations.H>
#include <list>
#include <string>

struct NioTab : public Fl_Group
{
    NioTab(std::string name);
    Fl_Light_Button outEnable;
    static void nioToggle(Fl_Widget *w, void *arg);
    void refresh();
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

