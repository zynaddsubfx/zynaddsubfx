/*
  ZynAddSubFX - a software synthesizer

  Fl_Osc_Dial.H - OSC Controlled Knob
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#pragma once
#include <FL/Fl_Dial.H>
#include "WidgetPDial.h"
#include "Fl_Osc_Widget.H"
#include <string>

class Fl_Osc_Dial:public WidgetPDial, public Fl_Osc_Widget
{

    public:
        Fl_Osc_Dial(int X, int Y, int W, int H, const char *label = NULL);
        virtual ~Fl_Osc_Dial(void);

        //Hack to get non-local controls up and running
        void alt_init(std::string base, std::string path_);


        //Normal Initialization
        void init(std::string path);
        //void OSC_value(float);
        void OSC_value(int);
        void OSC_value(char);
        void OSC_value(float);
        using Fl_Osc_Widget::OSC_value;

        //Refetch parameter information
        void update(void);
        void callback(Fl_Callback *cb, void *p = NULL);

        //Midi learn handler
        int handle(int);

        void mark_dead(void);
        virtual void rebase(std::string new_base) override;

        void cb(void);

        //! normally, it's all uchar. this hack makes it a float dial
        void make_float() { dataType = dataTypeT::type_float; }
    private:
        enum class dataTypeT
        {
            type_uchar, type_float
        } dataType = dataTypeT::type_uchar;
        bool alt_style;
        bool dead;
        std::pair<Fl_Callback*, void*> cb_data;
};
