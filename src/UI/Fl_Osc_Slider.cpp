#include "Fl_Osc_Slider.H"
#include "Fl_Osc_Interface.h"
#include "Fl_Osc_Pane.H"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>

static double min__(double a, double b)
{
    return a<b?a:b;
}

Fl_Osc_Slider::Fl_Osc_Slider(int X, int Y, int W, int H, const char *label)
    :Fl_Slider(X,Y,W,H,label), Fl_Osc_Widget(this), cb_data(NULL, NULL)
{
    //bounds(0.0f,1.0f);
    Fl_Slider::callback(Fl_Osc_Slider::_cb);
}

void Fl_Osc_Slider::init(std::string path_, char type_)
{
    osc_type = type_;
    ext = path_;
    oscRegister(ext.c_str());
}

Fl_Osc_Slider::~Fl_Osc_Slider(void)
{}

void Fl_Osc_Slider::OSC_value(int v)
{
    const float min_ = min__(minimum(), maximum());//flipped sliders
    Fl_Slider::value(v+min_);
}

void Fl_Osc_Slider::OSC_value(float v)
{
    const float min_ = min__(minimum(), maximum());//flipped sliders
    Fl_Slider::value(v+min_);
}

void Fl_Osc_Slider::OSC_value(char v)
{
    const float min_ = min__(minimum(), maximum());//flipped sliders
    Fl_Slider::value(v+min_);
}

void Fl_Osc_Slider::cb(void)
{
    const float min_ = min__(minimum(), maximum());//flipped sliders
    const float val = Fl_Slider::value();
    if(osc_type == 'f')
        oscWrite(ext, "f", val-min_);
    else if(osc_type == 'i')
        oscWrite(ext, "i", (int)(val-min_));
    else
        oscWrite(ext, "c", (char)(val-min_));
    //OSC_value(val);
    
    if(cb_data.first)
        cb_data.first(this, cb_data.second);
}

void Fl_Osc_Slider::callback(Fl_Callback *cb, void *p)
{
    cb_data.first = cb;
    cb_data.second = p;
}

void Fl_Osc_Slider::update(void)
{
    oscWrite(ext, "");
}

void Fl_Osc_Slider::_cb(Fl_Widget *w, void *)
{
    static_cast<Fl_Osc_Slider*>(w)->cb();
}
