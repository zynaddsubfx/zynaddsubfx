#include "Fl_Osc_Choice.H"
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

static Fl_Osc_Pane *fetch_osc_pane(Fl_Widget *w)
{
    if(!w)
        return NULL;

    Fl_Osc_Pane *pane = dynamic_cast<Fl_Osc_Pane*>(w->parent());
    if(pane)
        return pane;
    return fetch_osc_pane(w->parent());
}

static void callback_fn(Fl_Widget *w, void *v)
{
    ((Fl_Osc_Choice*)w)->cb();
}

Fl_Osc_Choice::Fl_Osc_Choice(int X, int Y, int W, int H, const char *label)
    :Fl_Choice(X,Y,W,H, label), Fl_Osc_Widget(), osc(NULL), cb_data(NULL, NULL)
{
    Fl_Choice::callback(callback_fn, NULL);
}

void Fl_Osc_Choice::init(const char *path)
{
    Fl_Osc_Pane *pane = fetch_osc_pane(this);
    assert(pane);
    osc = pane->osc;
    full_path = pane->pane_name + path;
    osc->createLink(full_path, this);
    osc->requestValue(full_path);
};

Fl_Osc_Choice::~Fl_Osc_Choice(void)
{
    if(osc)
        osc->removeLink(full_path, this);
}
        
void Fl_Osc_Choice::callback(Fl_Callback *cb, void *p)
{
    cb_data.first = cb;
    cb_data.second = p;
}

void Fl_Osc_Choice::OSC_value(char v)
{
    value(v);
//    real_value = v;
//    const float val = Fl_Osc_Widget::inv_translate(v, metadata.c_str());
//    Fl_Dial::value(val);
//label_str = string_cast<int,string>(v);
//    label("                ");
//    label(label_str.c_str());
}

void Fl_Osc_Choice::cb(void)
{
    assert(osc);
    osc->writeValue(full_path, (char)(value()));
    if(cb_data.first)
        cb_data.first(this, cb_data.second);
}

void Fl_Osc_Choice::update(void)
{
    assert(osc);
    osc->requestValue(full_path);
}

