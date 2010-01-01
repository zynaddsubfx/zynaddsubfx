#include "NioUI.h"
#include "../Nio/EngineMgr.h"
#include "../Nio/OutMgr.h"
#include "../Nio/AudioOut.h"
#include <cstdio>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Text_Display.H>
#include <iostream>
#include <cstring>

using namespace std;

NioUI::NioUI()
    :Fl_Window(200,100,400,400,"New IO Controls")
{
    //hm, I appear to be leaking memory
    Fl_Tabs *wintabs = new Fl_Tabs(0,0,400,400-15);
    {
        Fl_Group *gen  = new Fl_Group(0,20,400,400-35,"General");
        {
            Fl_Text_Buffer  *buff  = new Fl_Text_Buffer();
            Fl_Text_Display *intro = new Fl_Text_Display(20,40,350,300);
            intro->buffer(buff);
            buff->text("Thanks For Testing Out the New"
                    " Input/Output system. "
                    "To get started, you may want to play with "
                    "the open output, which is highted in green."
                    "Beware of bugs that may exist and"
                    " enjoy the new system.");
            intro->wrap_mode(4, 40);
        }
        gen->end();

        for(list<Engine *>::iterator itr = sysEngine->engines.begin();
                itr != sysEngine->engines.end(); ++itr)
            tabs.push_back(new NioTab((*itr)->name));

        //add tabs
        for(list<NioTab *>::iterator itr = tabs.begin();
                itr != tabs.end(); ++itr)
            wintabs->add(*itr);
    }
    wintabs->end();

    Fl::scheme("plastic");
    resizable(this);
    size_range(400,300);
}

void NioUI::refresh()
{
    for(list<NioTab *>::iterator itr = tabs.begin();
            itr != tabs.end(); ++itr)
        (*itr)->refresh();
}

void NioTab::nioToggle(Fl_Widget *wid, void *arg)
{
    Fl_Button *w = static_cast<Fl_Button *>(wid);
    NioTab *p = static_cast<NioTab *>(arg);
    bool val = w->value();

    AudioOut *out = sysOut->getOut(p->name);
    if(out) {
        if(val)
            out->Start();
        else
            out->Stop();
    }
    p->refresh();
}

void NioTab::nioBuffer(Fl_Widget *wid, void *arg)
{
    Fl_Spinner *w = static_cast<Fl_Spinner *>(wid);
    char *str = static_cast<char *>(arg);
    int val = (int) w->value();

    cout << "Chaning Buffer Size For " << str << endl;
    AudioOut *out = sysOut->getOut(str);
    if(out) {
        cout << "Chaning Buffer Size To " << val << endl;
        out->bufferingSize(val);
    }
}

NioTab::NioTab(string name)
    :Fl_Group(10, 40, 400, 400-35, strdup(name.c_str())),
    outEnable(20, 30, 100, 25, "Enable"),
    buffer(70, 60, 50, 25, "Buffer:"),//in SOUND_BUFFER_SIZE units
    name(name)
{
    outEnable.callback(nioToggle, (void *)this);
    buffer.callback(nioBuffer, (void *)name.c_str());
    //this is a COMPLETELY arbitrary max
    //I just assume that users do not want an overly long buffer
    buffer.range(1, 100);
    buffer.type(FL_INT_INPUT);
    end();
}

void NioTab::refresh()
{
    //getOut should only be called with present Engines
    bool state = sysOut->getOut(name)->isEnabled();
    outEnable.value(state);
    buffer.value(sysOut->getOut(name)->bufferingSize());

    this->labelcolor(fl_rgb_color(0,255*state,0));
    this->redraw();
}

