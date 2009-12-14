#include <FL/Fl.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Pack.H>
#include <FL/Enumerations.H>

class Pack : public Fl_Pack
{
    public:
        Pack(int x, int y, int w, int h);
    private:
        Fl_Light_Button b1,b2,b3;
};

Pack::Pack(int x, int y, int w, int h)
    :Fl_Pack(x,y,w,h),
    //b1(x+10,y+5,x+100,y+25,"A"),
    //b2(x+10,y+30,x+100,y+25,"B"),
    //b3(x+10,y+60,x+000,y+25,"C")
    b1(0,0,100,25,"Null I/O"),
    b2(0,0,100,25,"ALSA I/O"),
    b3(0,0,100,25,"OSS  I/O")
{
    b1.selection_color(fl_rgb_color(0,255,0));
    b2.selection_color(fl_rgb_color(0,255,0));
    b3.selection_color(fl_rgb_color(0,255,0));
}

int main()
{
    Fl::scheme("plastic");
    Fl_Window win(200,100,400,400,"New IO Controls");
    win.resizable(&win);
    win.size_range(400,300);
//    Fl_Light_Button button(10, 5, 100, 25, "NullEngine");
//    button.color(fl_rgb_color(255,0,0),fl_rgb_color(0,255,0));//)
//    Fl_Button buta(10,50,100,25,"Enable");
    Pack foo(20,50,100,25);
    win.show();
    return Fl::run();
}
