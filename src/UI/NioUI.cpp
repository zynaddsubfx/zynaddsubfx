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
       
        int audioval = 0;
        int midival  = 0;
        for(list<Engine *>::iterator itr = sysEngine->engines.begin();
                itr != sysEngine->engines.end(); ++itr) {
            Engine *eng = *itr;
            if(dynamic_cast<MidiIn *>(eng))
                midi->add(eng->name.c_str());
            if(dynamic_cast<AudioOut *>(eng))
                audio->add(eng->name.c_str());
            if(eng->name == sysOut->getSink()) {
                audioval = audio->size() - 2;
                cout << "Setting audio value to " << audio->size() << endl;
            }
            if(eng->name == sysIn->getSource()) {
                midival = midi->size() - 2;
                cout << "Setting midi value to " << midi->size() << endl;
            }
        }
        audio->value(audioval);
        midi->value(midival);
    }
    wintabs->end();

    resizable(this);
    size_range(400,300);
}

void NioUI::refresh()
{
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

