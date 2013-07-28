#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Double_Window.H>
#include <string>

class Fl_Osc_Pane
{
    public:
        Fl_Osc_Pane(void)
            :osc(NULL), pane_name()
        {}
        class Fl_Osc_Interface *osc;
        std::string pane_name;
};

class Fl_Osc_Window:public Fl_Osc_Pane, public Fl_Double_Window
{
    public:
        Fl_Osc_Window(int w, int h, const char *L=0)
            :Fl_Double_Window(w,h,L)
        {}

        void init(Fl_Osc_Interface *osc_, std::string loc_)
        {
            osc       = osc_;
            pane_name = loc_;
        }
};

class Fl_Osc_Group:public Fl_Osc_Pane, public Fl_Group
{
    public:
        Fl_Osc_Group(int x, int y, int w, int h, const char *L=0)
            :Fl_Group(x,y,w,h,L)
        {
            if(auto *p = dynamic_cast<Fl_Osc_Pane*>(parent())) {
                osc = p->osc;
                pane_name = p->pane_name;
            }
        };
};


