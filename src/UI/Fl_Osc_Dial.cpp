#include "Fl_Osc_Dial.H"
#include "Fl_Osc_Interface.h"
#include "Fl_Osc_Pane.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>

static void callback_fn(Fl_Widget *w, void *)
{
    ((Fl_Osc_Dial*)w)->cb();
}

Fl_Osc_Pane *fetch_osc_pane(Fl_Widget *w)
{
    if(!w)
        return NULL;

    Fl_Osc_Pane *pane = dynamic_cast<Fl_Osc_Pane*>(w->parent());
    if(pane)
        return pane;
    return fetch_osc_pane(w->parent());
}

Fl_Osc_Dial::Fl_Osc_Dial(int X, int Y, int W, int H, const char *label)
    :WidgetPDial(X,Y,W,H, label), Fl_Osc_Widget(this)
{
    bounds(0.0, 127.0f);
    WidgetPDial::callback(callback_fn);
}


void Fl_Osc_Dial::init(const char *path_)
{
    assert(osc);
    path = path_;
    full_path = loc + path;
    oscRegister(path_);
};
        
void Fl_Osc_Dial::alt_init(std::string base, std::string path_)
{
    Fl_Osc_Pane *pane = fetch_osc_pane(this);
    assert(pane);
    osc = pane->osc;
    assert(osc);
    loc  = base;
    full_path = loc + path_;
    oscRegister(path_.c_str());
}

Fl_Osc_Dial::~Fl_Osc_Dial(void)
{}

void Fl_Osc_Dial::callback(Fl_Callback *cb, void *p)
{
    cb_data.first = cb;
    cb_data.second = p;
}

void Fl_Osc_Dial::OSC_value(int v)
{
    value(v+minimum());
}

void Fl_Osc_Dial::OSC_value(char v)
{
    value(v+minimum());
}
        
void Fl_Osc_Dial::update(void)
{
    osc->requestValue(full_path);
}

void Fl_Osc_Dial::cb(void)
{
    assert(osc);

    if((maximum()-minimum()) == 127 || (maximum()-minimum()) == 255)
        oscWrite(path, "c", (char)(value()-minimum()));
    else
        oscWrite(path, "i", (int)(value()-minimum()));
    
    if(cb_data.first)
        cb_data.first(this, cb_data.second);
}
