#include "PartNameButton.h"

PartNameButton::PartNameButton(int X, int Y, int W, int H, const char *label)
    :Fl_Button(X,Y,W,H,label), Fl_Osc_Widget(this)
{
}

void PartNameButton::OSC_value(const char *label_)
{
    printf("\n\n\n\n\n\n\n\n\nNWEWERWERW LABESRL:\n");
    printf("'%s'\n", label_);

    the_string = label_;
    label(the_string.c_str());
}

void PartNameButton::debug(void)
{
    printf("debugging part name at address '%x'\n", (int)this);
    Fl_Group *w = this->parent();
    while(w)
    {
        printf("    Looking at parent '%x'[%d,%d]\n", (int)w, w->visible(), w->visible_r());
        if(auto *win = dynamic_cast<Fl_Window*>(w))
            printf("        Window(%d,'%s')\n", win->shown(), win->label());
        w = w->parent();
    }
}
