#pragma once
#include <FL/Fl_Button.H>
#include "Fl_Osc_Widget.H"
#include <string>

class PartNameButton:public Fl_Button, public Fl_Osc_Widget
{
    public:
        PartNameButton(int X, int Y, int W, int H, const char *label=NULL);

        virtual ~PartNameButton(void){};
        virtual void OSC_value(const char *);
        void debug(void);
        std::string the_string;

        //virtual void rebase(std::string) override;
};
