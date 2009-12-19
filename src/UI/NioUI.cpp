#include "NioUI.h"
#include "../Nio/OutMgr.h"
#include "../Nio/AudioOut.h"
#include <cstdio>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Text_Display.H>


NioUI::NioUI()
    :Fl_Window(200,100,400,400,"New IO Controls"),
    groups(new Fl_Group*[4]), buttons(new Fl_Button*[4])
{
    groups[0]=NULL;
    groups[1]=NULL;
    groups[2]=NULL;
    groups[3]=NULL;

    jackc = "JACK";
    ossc  = "OSS";
    alsac = "ALSA";
    nullc = "NULL";


    //hm, I appear to be leaking memory
    Fl_Tabs *tabs = new Fl_Tabs(0,0,400,400-15);
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
        Fl_Group *oss  = new Fl_Group(10,40,400,400-35,ossc);
        {
            Fl_Light_Button *enabler = new Fl_Light_Button(20,30,100,25,"Enable");
            enabler->callback(nioToggle, (void *)oss);
            //enabler->value((NULL==sysOut->getOut(ossc)?false:
            //            sysOut->getOut(ossc)->isEnabled()));
            //if(sysOut->getOut(ossc)!=NULL&&sysOut->getOut(ossc)->isEnabled())
            //{
            //    oss->labelcolor(fl_rgb_color(0,255,0));
            //    oss->redraw();
            //}
            groups[0]  = oss;
            buttons[0] = enabler;
        }
        oss->end();
        Fl_Group *alsa = new Fl_Group(0,20,400,400-35,alsac);
        {
            Fl_Light_Button *enabler = new Fl_Light_Button(20,30,100,25,"Enable");
            enabler->callback(nioToggle, (void *)alsa);
            //enabler->value((NULL==sysOut->getOut(alsac)?false:
            //            sysOut->getOut(alsac)->isEnabled()));
            //if(sysOut->getOut(alsac)!=NULL&&sysOut->getOut(alsac)->isEnabled())
            //{
            //    alsa->labelcolor(fl_rgb_color(0,255,0));
            //    alsa->redraw();
            //}
            groups[1]  = alsa;
            buttons[1] = enabler;
        }
        alsa->end();
        Fl_Group *jack = new Fl_Group(0,20,400,400-35,jackc);
        {
            Fl_Light_Button *enabler = new Fl_Light_Button(20,30,100,25,"Enable");
            enabler->callback(nioToggle, (void *)jack);
            //enabler->value((NULL==sysOut->getOut(jackc)?false:
            //            sysOut->getOut(jackc)->isEnabled()));
            //if(sysOut->getOut(jackc)!=NULL&&sysOut->getOut(jackc)->isEnabled())
            //{
            //    jack->labelcolor(fl_rgb_color(0,255,0));
            //    jack->redraw();
            //}
            groups[2]  = jack;
            buttons[2] = enabler;
        }
        jack->end();
        Fl_Group *null = new Fl_Group(0,20,400,400-35,nullc);
        {
            Fl_Light_Button *enabler = new Fl_Light_Button(20,30,100,25,"Enable");
            enabler->callback(nioToggle, (void *)null);
            //enabler->value((NULL==sysOut->getOut(nullc)?false:
            //            sysOut->getOut(nullc)->isEnabled()));
            //if(sysOut->getOut(nullc)!=NULL&&sysOut->getOut(nullc)->isEnabled())
            //{
            //    null->labelcolor(fl_rgb_color(0,255,0));
            //    null->redraw();
            //}
            groups[3]  = null;
            buttons[3] = enabler;
        }
        null->end();
    }
    tabs->end();

    Fl::scheme("plastic");
    resizable(this);
    size_range(400,300);
}

void NioUI::refresh()
{
    for(int i = 0; i < 4; ++i)
        if(groups[i])
            nioToggle(buttons[i],groups[i]);
}

//this is a bit of a hack (but a fun one)
void NioUI::nioToggle(Fl_Widget *wid, void *parent)
{
    Fl_Button *w = static_cast<Fl_Button *>(wid);
    Fl_Group  *p = static_cast<Fl_Group *>(parent);
    bool val = w->value();
    
    AudioOut *out = sysOut->getOut(p->label());
    if(!out)
        return;

    bool result=false;

    if(val)
        result=out->Start();
    else
        out->Stop();

    w->value(result);
    p->labelcolor(fl_rgb_color(0,result*255,0));
    p->redraw();
}

