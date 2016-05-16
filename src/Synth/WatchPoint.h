/*
  ZynAddSubFX - a software synthesizer

  WatchPoint.h - Synthesis State Watcher
  Copyright (C) 2015-2015 Mark McCurry

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#pragma once

class WatchManager;
namespace rtosc {class ThreadLink;}

struct WatchPoint
{
    bool          active;
    int           samples_left;
    WatchManager *reference;
    char          identity[128];

    WatchPoint(WatchManager *ref, const char *prefix, const char *id);
    bool is_active(void);
};

#define MAX_WATCH 16
struct WatchManager
{
    typedef rtosc::ThreadLink thrlnk;
    thrlnk *write_back;
    bool    new_active;
    char    active_list[128][MAX_WATCH];
    int     sample_list[MAX_WATCH];
    bool    deactivate[MAX_WATCH];

    //External API
    WatchManager(thrlnk *link=0);
    void add_watch(const char *);
    void del_watch(const char *);
    void tick(void);

    //Watch Point Query API
    bool active(const char *) const;
    int  samples(const char *) const;

    //Watch Point Response API
    void satisfy(const char *, float);
};

struct FloatWatchPoint:public WatchPoint
{
    FloatWatchPoint(WatchManager *ref, const char *prefix, const char *id);
    inline void operator()(float f)
    {
        if(is_active() && reference) {
            reference->satisfy(identity, f);
            active = false;
        }
    }
};

//struct VecWatchPoint:public WatchPoint
//{
//    inline void operator()(float *f, int n)
//    {
//        if(!is_active()) {
//        }
//    }
//};
