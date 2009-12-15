#include "NioUI.h"

Pack::Pack(int x, int y, int w, int h)
    :Fl_Pack(x,y,w,h),
    b1(0,0,100,25,"Null I/O"),
    b2(0,0,100,25,"ALSA I/O"),
    b3(0,0,100,25,"OSS  I/O")
{
    b1.selection_color(fl_rgb_color(0,255,0));
    b2.selection_color(fl_rgb_color(0,255,0));
    b3.selection_color(fl_rgb_color(0,255,0));
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
