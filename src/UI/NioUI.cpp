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
        static void nioTogglei(Fl_Widget *wid, void *name);
};

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

class NioUI : public Fl_Window
{
    public:
        NioUI()
            :Fl_Window(200,100,400,400,"New IO Controls"),
             foo(20,50,100,25)
        {
            Fl::scheme("plastic");
            resizable(this);
            size_range(400,300);
            show();
        }
    private:
    Pack foo;
};



int main()
{
    NioUI myUI;
    return Fl::run();
}
