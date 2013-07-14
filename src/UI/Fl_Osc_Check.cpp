#include "Fl_Osc_Check.H"
#include "Fl_Osc_Interface.h"
#include "Fl_Osc_Pane.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>

Fl_Osc_Check::Fl_Osc_Check(int X, int Y, int W, int H, const char *label)
    :Fl_Check_Button(X,Y,W,H,label), Fl_Osc_Widget()
{
    callback(Fl_Osc_Check::_cb);
}

Fl_Osc_Check::~Fl_Osc_Check(void)
{
    osc->removeLink(full_path, this);
}

void Fl_Osc_Check::OSC_value(bool v)
{
    Fl_Check_Button::value(v);
}

void Fl_Osc_Check::init(std::string path)
{
    Fl_Osc_Pane *pane = fetch_osc_pane(this);
    assert(pane);
    osc = pane->osc;
    init(osc,path);
}

void Fl_Osc_Check::init(Fl_Osc_Interface *osc, std::string path)
{
    Fl_Osc_Pane *pane = fetch_osc_pane(this);
    full_path = pane->pane_name + path;
    osc->createLink(full_path, this);
    osc->requestValue(full_path);
}

void Fl_Osc_Check::cb(void)
{
    osc->writeValue(full_path, (bool) value());
}

void Fl_Osc_Check::update(void)
{}

void Fl_Osc_Check::_cb(Fl_Widget *w, void *)
{
    static_cast<Fl_Osc_Check*>(w)->cb();
}
