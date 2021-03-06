/*
  ZynAddSubFX - a software synthesizer

  EnvelopeFreeEdit.cpp - Envelope Edit View
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "EnvelopeFreeEdit.h"
#include "../Misc/Util.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <cstdlib>
#include <cassert>
#include <rtosc/rtosc.h>

using namespace zyn;

EnvelopeFreeEdit::EnvelopeFreeEdit(int x,int y, int w, int h, const char *label)
:Fl_Box(x,y,w,h,label), Fl_Osc_Widget(this)
{
    pair=NULL;
    currentpoint=-1;
    cpx=0;
    lastpoint=-1;
}

void EnvelopeFreeEdit::init(void)
{
    oscRegister("Penvpoints");
    oscRegister("Penvdt");
    oscRegister("Penvval");
    oscRegister("Penvsustain");

    //register for non-bulk types
    for(int i=0; i<MAX_ENVELOPE_POINTS; ++i) {
        osc->createLink(loc+string("Penvdt") + to_s(i), this);
        osc->createLink(loc+string("Penvval") + to_s(i), this);
    }
}

void EnvelopeFreeEdit::OSC_raw(const char *msg)
{
    const char *args = rtosc_argument_string(msg);
    if(strstr(msg,"Penvpoints") && !strcmp(args, "i")) {
        Penvpoints = rtosc_argument(msg, 0).i;
    } else if(strstr(msg,"Penvdt") && !strcmp(args, "b")) {
        rtosc_blob_t b = rtosc_argument(msg, 0).b;
        assert(b.len == MAX_ENVELOPE_POINTS);
        memcpy(Penvdt, b.data, MAX_ENVELOPE_POINTS);
    } else if(strstr(msg,"Penvval") && !strcmp(args, "b")) {
        rtosc_blob_t b = rtosc_argument(msg, 0).b;
        assert(b.len == MAX_ENVELOPE_POINTS);
        memcpy(Penvval, b.data, MAX_ENVELOPE_POINTS);
    } else if(strstr(msg, "Penvval") && !strcmp(args, "i")) {
        const char *str = strstr(msg, "Penvval");
        int id = atoi(str+7);
        assert(0 <= id && id < MAX_ENVELOPE_POINTS);
        Penvval[id] = rtosc_argument(msg, 0).i;
    } else if(strstr(msg, "Penvdt") && !strcmp(args, "i")) {
        const char *str = strstr(msg, "Penvdt");
        int id = atoi(str+6);
        assert(0 <= id && id < MAX_ENVELOPE_POINTS);
        Penvdt[id] = rtosc_argument(msg, 0).i;
    } else if(strstr(msg,"Penvsustain") && !strcmp(args, "i")) {
        Penvsustain = rtosc_argument(msg, 0).i;
    }
    redraw();
    do_callback();
}

void EnvelopeFreeEdit::setpair(Fl_Box *pair_)
{
    pair=pair_;
}

int EnvelopeFreeEdit::getpointx(int n) const
{
    const int lx=w()-10;
    int npoints=Penvpoints;

    float  sum=0;
    for(int i=1; i<npoints; ++i)
        sum+=getdt(i)+1;

    float sumbefore=0;//the sum of all points before the computed point
    for(int i=1; i<=n; ++i)
        sumbefore+=getdt(i)+1;

    return (int) (sumbefore/(float) sum*lx);
}

int EnvelopeFreeEdit::getpointy(int n) const
{
    const int ly=h()-10;

    return (1.0-Penvval[n]/127.0)*ly;
}

static inline int distance_fn(int dx, int dy) {
    return dx*dx+dy*dy;
}

int EnvelopeFreeEdit::getnearest(int x,int y) const
{
    x-=5;y-=5;

    int nearestpoint=0;
    int nearest_distance_sq=distance_fn(x-getpointx(0), y-getpointy(0));
    for(int i=1; i<Penvpoints; ++i){
        int distance_sq=distance_fn(x-getpointx(i), y-getpointy(i));
        if (distance_sq<nearest_distance_sq) {
            nearestpoint=i;
            nearest_distance_sq=distance_sq;
        }
    }

    return nearestpoint;
}

static float dt(char val)
{
    return (powf(2.0f, val / 127.0f * 12.0f) - 1.0f) * 10.0f; //milliseconds
}

float EnvelopeFreeEdit::getdt(int i) const
{
    return dt(Penvdt[i]);
}

static bool ctrldown, altdown;

void EnvelopeFreeEdit::draw(void)
{
    int ox=x(),oy=y(),lx=w(),ly=h();
    //if (env->Pfreemode==0)
    //    env->converttofree();
    const int npoints=Penvpoints;

    if (active_r()) fl_color(FL_BLACK);
    else fl_color(90,90,90);
    if (!active_r()) currentpoint=-1;

    fl_rectf(ox,oy,lx,ly);

    //Margins
    ox+=5;oy+=5;lx-=10;ly-=10;

    //draw the lines
    fl_color(FL_GRAY);

    const int midline = oy+ly*(1-64.0/127);
    fl_line_style(FL_SOLID);
    fl_line(ox+2,midline,ox+lx-2,midline);

    //draws the envelope points and lines
    Fl_Color alb=FL_WHITE;
    if (!active_r()) alb=fl_rgb_color(180,180,180);
    fl_color(alb);
    int oldxx=0,xx=0,oldyy=0,yy=getpointy(0);
    fl_rectf(ox-3,oy+yy-3,6,6);
    for (int i=1; i<npoints; ++i){
        oldxx=xx;oldyy=yy;
        xx=getpointx(i);yy=getpointy(i);
        if (i==currentpoint || (ctrldown && i==lastpoint))
            fl_color(FL_RED);
        else
            fl_color(alb);
        fl_line(ox+oldxx,oy+oldyy,ox+xx,oy+yy);
        fl_rectf(ox+xx-3,oy+yy-3,6,6);
    }

    //draw the last moved point point (if exists)
    if (lastpoint>=0){
        fl_color(FL_CYAN);
        fl_rectf(ox+getpointx(lastpoint)-5,oy+getpointy(lastpoint)-5,10,10);
    }

    //draw the sustain position
    if(Penvsustain>0){
        fl_color(FL_YELLOW);
        xx=getpointx(Penvsustain);
        fl_line(ox+xx,oy+0,ox+xx,oy+ly);
    }

    //Show the envelope duration and the current line duration
    fl_font(FL_HELVETICA|FL_BOLD,10);
    float time=0.0;
    if (currentpoint<=0 && (!ctrldown||lastpoint <= 0)){
        fl_color(alb);
        for(int i=1; i<npoints; ++i)
            time+=getdt(i);
    } else {
        fl_color(FL_RED);
        time=getdt(lastpoint);
    }
    char tmpstr[20];
    if (!altdown || ctrldown) {
        if (time<1000.0)
            snprintf((char *)&tmpstr,20,"%.1fms",time);
        else
            snprintf((char *)&tmpstr,20,"%.2fs",time/1000.0);
        fl_draw(tmpstr,ox+lx-20,oy+ly-10,20,10,FL_ALIGN_RIGHT,NULL,0);
    }
    if (!altdown || !ctrldown) {
        if (lastpoint>=0){
            snprintf((char *)&tmpstr,20,"%d", Penvval[lastpoint]);
            fl_draw(tmpstr,ox+lx-20,oy+ly-23,20,10,FL_ALIGN_RIGHT,NULL,0);
        }
    }
}

int EnvelopeFreeEdit::handle(int event)
{
    const int x_=Fl::event_x()-x();
    const int y_=Fl::event_y()-y();
    static Fl_Widget *old_focus;
    int key, old_mod_state;

    switch(event) {
      case FL_ENTER:
          old_focus=Fl::focus();
          Fl::focus(this);
          // Otherwise the underlying window seems to regrab focus,
          // and I can't see the KEYDOWN action.
          return 1;
      case FL_LEAVE:
          Fl::focus(old_focus);
          break;
      case FL_KEYDOWN:
      case FL_KEYUP:
          key = Fl::event_key();
          if (key==FL_Alt_L || key==FL_Alt_R) {
              altdown = (event==FL_KEYDOWN);
              redraw();
              if (pair!=NULL) pair->redraw();
          }
          if (key==FL_Control_L || key==FL_Control_R){
              ctrldown = (event==FL_KEYDOWN);
              redraw();
              if (pair!=NULL) pair->redraw();
          }
          break;
      case FL_PUSH:
            currentpoint=getnearest(x_,y_);
            cpx=x_;
            cpy=y_;
            cpdt=Penvdt[currentpoint];
            cpval=Penvval[currentpoint];
            lastpoint=currentpoint;
            redraw();
            if (pair)
                pair->redraw();
            return 1;
      case FL_RELEASE:
            currentpoint=-1;
            redraw();
            if (pair)
                pair->redraw();
            return 1;
      case FL_MOUSEWHEEL:
          if (Fl::event_buttons())
              return 1;
          if (lastpoint>=0) {
              int delta = Fl::event_dy() * (Fl::event_shift() ? 4 : 1);
              if (!ctrldown) {
                  int ny = Penvval[lastpoint] - delta;
                  ny = ny < 0 ? 0 : ny > 127 ? 127 : ny;
                  Penvval[lastpoint] = ny;
                  oscWrite(to_s("Penvval")+to_s(lastpoint), "i", ny);
                  oscWrite("Penvval","");
              } else if (lastpoint > 0) {
                  int newdt = Penvdt[lastpoint] - delta;
                  newdt = newdt < 0 ? 0 : newdt > 127 ? 127 : newdt;
                  Penvdt[lastpoint] = newdt;
                  oscWrite(to_s("Penvdt")+to_s(lastpoint),  "i", newdt);
                  oscWrite("Penvdt","");
              }
              redraw();
              if (pair!=NULL) pair->redraw();
              return 1;
          }
          // fall through
      case FL_DRAG:
          if (currentpoint>=0){
              old_mod_state = mod_state;
              mod_state = ctrldown << 1 | altdown;
              if (old_mod_state != mod_state) {
                  cpx=x_;
                  cpy=y_;
                  cpdt=Penvdt[currentpoint];
                  cpval=Penvval[currentpoint];
                  old_mod_state = mod_state;
              }

              if (!altdown || !ctrldown) {
                  const int dy=(int)((cpy-y_)/3.0);
                  const int newval=limit(cpval+dy, 0, 127);

                  Penvval[currentpoint]=newval;
                  oscWrite(to_s("Penvval")+to_s(currentpoint), "i", newval);
                  oscWrite("Penvval","");
              }

              if (!altdown || ctrldown) {
                  const int dx=(int)((x_-cpx)*0.1);
                  const int newdt=limit(cpdt+dx,0,127);

                  if(currentpoint!=0)
                      Penvdt[currentpoint]=newdt;
                  else
                      Penvdt[currentpoint]=0;
                  oscWrite(to_s("Penvdt")+to_s(currentpoint),  "i", newdt);
                  oscWrite("Penvdt","");
              }

              redraw();

              if(pair)
                  pair->redraw();
              return 1;
          }
    }
      // Needed to propagate undo/redo keys.
    return 0;
}

void EnvelopeFreeEdit::update(void)
{
    oscWrite("Penvpoints");
    oscWrite("Penvdt");
    oscWrite("Penvval");
    oscWrite("Penvsustain");
}

void EnvelopeFreeEdit::rebase(std::string new_base)
{
    osc->renameLink(loc+"Penvpoints", new_base+"Penvpoints", this);
    osc->renameLink(loc+"Penvdt", new_base+"Penvdt", this);
    osc->renameLink(loc+"Penvval",    new_base+"Penvval", this);
    osc->renameLink(loc+"Penvsustain", new_base+"Penvsustain", this);
    for(int i=0; i<MAX_ENVELOPE_POINTS; ++i) {
        string dt  = string("Penvdt")  + to_s(i);
        string val = string("Penvval") + to_s(i);
        osc->renameLink(loc+dt, new_base+dt, this);
        osc->renameLink(loc+val, new_base+val, this);
    }
    loc = new_base;
    update();
}
