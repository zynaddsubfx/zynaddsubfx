#include "NioUI.h"
#include "../Nio/EngineMgr.h"
#include "../Nio/OutMgr.h"
#include "../Nio/InMgr.h"
#include "../Nio/AudioOut.h"
#include "../Nio/MidiIn.h"
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

        Fl_Group *settings = new Fl_Group(0,20,400,400-35,"Settings");
        {
            audio = new Fl_Choice(60, 80, 100, 25, "Audio");
            audio->callback(audioCallback);
            midi = new Fl_Choice(60, 100, 100, 25, "Midi");
            midi->callback(midiCallback);
        }
        settings->end();
       
        for(list<Engine *>::iterator itr = sysEngine->engines.begin();
                itr != sysEngine->engines.end(); ++itr) {
            Engine *eng = *itr;
            if(dynamic_cast<MidiIn *>(eng))
                midi->add(eng->name.c_str());
            if(dynamic_cast<AudioOut *>(eng))
                audio->add(eng->name.c_str());
        }


       //for(list<Engine *>::iterator itr = sysEngine->engines.begin();
       //        itr != sysEngine->engines.end(); ++itr) {
       //     bool midi  = dynamic_cast<MidiIn *>(*itr);
       //     bool audio = dynamic_cast<AudioOut *>(*itr);
       //     tabs.push_back(new NioTab((*itr)->name, midi, audio));
       //}

        //add tabs
        //for(list<NioTab *>::iterator itr = tabs.begin();
        //        itr != tabs.end(); ++itr)
        //    wintabs->add(*itr);
    }
    wintabs->end();

    resizable(this);
    size_range(400,300);
}

void NioUI::refresh()
{
    //for(list<NioTab *>::iterator itr = tabs.begin();
    //        itr != tabs.end(); ++itr)
    //    (*itr)->refresh();
    
}

void NioUI::midiCallback(Fl_Widget *c)
{
    bool good = sysIn->setSource(static_cast<Fl_Choice *>(c)->text());
    static_cast<Fl_Choice *>(c)->textcolor(fl_rgb_color(255*!good,0,0));
}

void NioUI::audioCallback(Fl_Widget *c)
{
    bool good = sysOut->setSink(static_cast<Fl_Choice *>(c)->text());
    static_cast<Fl_Choice *>(c)->textcolor(fl_rgb_color(255*!good,0,0));
}
#if 0
//this is a repetitve block of code
//perhaps something on the Engine's side should be refactored
void NioTab::nioToggle(Fl_Widget *wid, void *arg)
{
    Fl_Button *w = static_cast<Fl_Button *>(wid);
    NioTab *p = static_cast<NioTab *>(arg);
    bool val = w->value();

    Engine *eng = sysEngine->getEng(p->name);
    if(eng) {
        if(val)
            eng->Start();
        else
            eng->Stop();
    }
    p->refresh();
}

void NioTab::audioToggle(Fl_Widget *wid, void *arg)
{
    Fl_Button *w = static_cast<Fl_Button *>(wid);
    NioTab *p = static_cast<NioTab *>(arg);
    bool val = w->value();

    AudioOut *out = sysOut->getOut(p->name);
    out->setAudioEn(val);
    p->refresh();
}

void NioTab::midiToggle(Fl_Widget *wid, void *arg)
{
    Fl_Button *w = static_cast<Fl_Button *>(wid);
    NioTab *p = static_cast<NioTab *>(arg);
    bool val = w->value();

    MidiIn *in = dynamic_cast<MidiIn *>(sysEngine->getEng(p->name));
    in->setMidiEn(val);
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

NioTab::NioTab(string name, bool _midi, bool _audio)
    :Fl_Group(10, 40, 400, 400-35, strdup(name.c_str())),
    enable(20, 30, 100, 25, "Enable"),
    audio(NULL), midi(NULL), buffer(NULL),
    name(name)
{
    enable.callback(nioToggle, (void *)this);
    if(_audio) {
        buffer = new Fl_Spinner(70, 60, 50, 25, "Buffer:");//in SOUND_BUFFER_SIZE units
        buffer->callback(nioBuffer, (void *)name.c_str());
        //this is a COMPLETELY arbitrary max
        //I just assume that users do not want an overly long buffer
        buffer->range(1, 100);
        buffer->type(FL_INT_INPUT);
        audio = new Fl_Light_Button(20, 80, 100, 25, "Audio");
        audio->callback(audioToggle, (void *)this);
    }
    if(_midi) {
        midi = new Fl_Light_Button(20, 100, 100, 25, "Midi");
        midi->callback(midiToggle, (void *)this);
    }

    end();
}

void NioTab::refresh()
{
    //get engine
    Engine *eng = sysEngine->getEng(name);
    MidiIn *midiIn = dynamic_cast<MidiIn *>(eng);
    AudioOut *audioOut = dynamic_cast<AudioOut *>(eng);
    if(midi)
        midi->value(midiIn->getMidiEn());
    if(audio)
        audio->value(audioOut->getAudioEn());

    bool state = eng->isRunning();
    enable.value(state);
    buffer->value(sysOut->getOut(name)->bufferingSize());


    this->labelcolor(fl_rgb_color(0,255*state,0));
    this->redraw();
}
#endif

