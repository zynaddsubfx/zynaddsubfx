#include "NioUI.h"
#include "../Nio/OutMgr.h"
#include "../Nio/AudioOut.h"
#include <cstdio>

Pack::Pack(int x, int y, int w, int h)
    :Fl_Pack(x,y,w,h),
    b1(0,0,100,25,"Null I/O"),
    b2(0,0,100,25,"ALSA I/O"),
    b3(0,0,100,25,"OSS  I/O")
{
    b1.selection_color(fl_rgb_color(0,255,0));
    b2.selection_color(fl_rgb_color(0,255,0));
    b3.selection_color(fl_rgb_color(0,255,0));
    b1.callback(nioToggle, (void *)"NULL");
    b2.callback(nioToggle, (void *)"ALSA");
    b3.callback(nioToggle, (void *)"OSS");
}

void Pack::nioToggle(Fl_Widget *w, void *name)
{
    bool val = static_cast<Fl_Button *>(w)->value();
    printf("test=%s %s\n",(const char *)name, (val?"enabling":"disableing"));
    w->active();
    AudioOut *out = sysOut->getOut((const char *) name);
    if(!out)
        return;
    if(val)
        out->Start();
    else
        out->Stop();
}

NioUI::NioUI()
    :Fl_Window(200,100,400,400,"New IO Controls"),
    foo(20,50,100,25)
{
    Fl::scheme("plastic");
    resizable(this);
    size_range(400,300);
}

//int main()
//{
//    NioUI myUI;
//    return Fl::run();
//}
