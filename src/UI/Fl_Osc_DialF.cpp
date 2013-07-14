#include "Fl_Osc_DialF.H"
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

static void callback_fn(Fl_Widget *w, void *v)
{
    ((Fl_Osc_DialF*)w)->cb();
}

static Fl_Osc_Pane *fetch_osc_pane(Fl_Widget *w)
{
    if(!w)
        return NULL;

    Fl_Osc_Pane *pane = dynamic_cast<Fl_Osc_Pane*>(w->parent());
    if(pane)
        return pane;
    return fetch_osc_pane(w->parent());
}

Fl_Osc_DialF::Fl_Osc_DialF(int X, int Y, int W, int H, const char *label)
    :WidgetPDial(X,Y,W,H, label), Fl_Osc_Widget()
{
    //bounds(0.0, 127.0f);
    WidgetPDial::callback(callback_fn);
}


void Fl_Osc_DialF::init(const char *path)
{
    Fl_Osc_Pane *pane = fetch_osc_pane(this);
    assert(pane);
    osc = pane->osc;
    full_path = pane->pane_name + path;
    osc->createLink(full_path, this);
    osc->requestValue(full_path);
};

Fl_Osc_DialF::~Fl_Osc_DialF(void)
{
    osc->removeLink(full_path, this);
}

void Fl_Osc_DialF::callback(Fl_Callback *cb, void *p)
{
    cb_data.first = cb;
    cb_data.second = p;
}

void Fl_Osc_DialF::OSC_value(float v)
{
    printf("Got a value of floating %f\n", v);
    value(v);
}
        
void Fl_Osc_DialF::update(void)
{
    osc->requestValue(full_path);
}

void Fl_Osc_DialF::cb(void)
{
    assert(osc);

    osc->writeValue(full_path, (float)value());
    
    if(cb_data.first)
        cb_data.first(this, cb_data.second);
//    label_str = string_cast<float,string>(val);
//    label("                ");
//    label(label_str.c_str());
}
