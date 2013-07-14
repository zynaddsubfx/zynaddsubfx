#include "Fl_Osc_Slider.H"
#include "Fl_Osc_Interface.h"
#include "Fl_Osc_Pane.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>

template<typename A, typename B>
B string_cast(const A &a)
{
    std::stringstream s;
    s.precision(3);
    B b;
    s << " " << a << " ";
    s >> b;
    return b;
}

Fl_Osc_Slider::Fl_Osc_Slider(int X, int Y, int W, int H, const char *label)
    :Fl_Slider(X,Y,W,H,label), Fl_Osc_Widget(), osc(NULL)
{
    //bounds(0.0f,1.0f);
    callback(Fl_Osc_Slider::_cb);
}

void Fl_Osc_Slider::init(std::string path)
{
    Fl_Osc_Pane *pane = fetch_osc_pane(this);
    assert(pane);
    osc = pane->osc;
    init(osc,path);
}

void Fl_Osc_Slider::init(Fl_Osc_Interface *osc, std::string path)
{
    Fl_Osc_Pane *pane = fetch_osc_pane(this);
    assert(pane);
    full_path = pane->pane_name + path;
    this->osc = osc;
    osc->createLink(full_path, this);
    osc->requestValue(full_path);
};

Fl_Osc_Slider::~Fl_Osc_Slider(void)
{
    if(osc)
        osc->removeLink(full_path, this);
    else
        fprintf(stderr, "Warning: Missing OSC link in " __FILE__ "\n");
}

void Fl_Osc_Slider::OSC_value(float v)
{
    Fl_Slider::value(v);
}

void Fl_Osc_Slider::cb(void)
{
    const float val = Fl_Slider::value();
    osc->writeValue(full_path, val);
    //OSC_value(val);
}

void Fl_Osc_Slider::_cb(Fl_Widget *w, void *)
{
    static_cast<Fl_Osc_Slider*>(w)->cb();
}
