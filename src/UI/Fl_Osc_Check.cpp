#include "Fl_Osc_Check.H"
#include "Fl_Osc_Interface.h"
#include "Fl_Osc_Pane.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>

Fl_Osc_Check::Fl_Osc_Check(int X, int Y, int W, int H, const char *label)
    :Fl_Check_Button(X,Y,W,H,label), Fl_Osc_Widget(this), cb_data(NULL, NULL)
{
    Fl_Check_Button::callback(Fl_Osc_Check::_cb);
}

Fl_Osc_Check::~Fl_Osc_Check(void)
{}

void Fl_Osc_Check::OSC_value(bool v)
{
    value(v);

    if(cb_data.first)
        cb_data.first(this, cb_data.second);
}

void Fl_Osc_Check::init(std::string path, char type)
{
    this->path = path;
    this->type = type;
    oscRegister(path.c_str());
}

void Fl_Osc_Check::cb(void)
{
    if(type == 'T')
        oscWrite(path, value() ? "T" : "F");
    else
        oscWrite(path, "c", value());

    if(cb_data.first)
        cb_data.first(this, cb_data.second);
}

void Fl_Osc_Check::update(void)
{}

void Fl_Osc_Check::callback(Fl_Callback *cb, void *p)
{
    cb_data.first = cb;
    cb_data.second = p;
}

void Fl_Osc_Check::_cb(Fl_Widget *w, void *)
{
    static_cast<Fl_Osc_Check*>(w)->cb();
}
