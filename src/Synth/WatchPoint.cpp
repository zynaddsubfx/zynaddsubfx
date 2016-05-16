/*
  ZynAddSubFX - a software synthesizer

  WatchPoint.cpp - Synthesis State Watcher
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

#include "WatchPoint.h"
#include <cstring>
#include <rtosc/thread-link.h>


WatchPoint::WatchPoint(WatchManager *ref, const char *prefix, const char *id)
    :active(false), samples_left(0), reference(ref)
{
    identity[0] = 0;
    if(prefix)
        strncpy(identity, prefix, 128);
    if(id)
        strncat(identity, id, 128);
    printf("new watchpoint ={%s:%s} <%s>\n", prefix, id, identity);
}

bool WatchPoint::is_active(void)
{
    //Either the watchpoint is already active or the watchpoint manager has
    //received another activation this frame
    if(active)
        return true;

    if(reference && reference->active(identity)) {
        active       = true;
        samples_left = reference->samples(identity);
        return true;
    }

    return false;
}
    
FloatWatchPoint::FloatWatchPoint(WatchManager *ref, const char *prefix, const char *id)
    :WatchPoint(ref, prefix, id)
{}
    
WatchManager::WatchManager(thrlnk *link)
    :write_back(link), new_active(false)
{
    memset(active_list, 0, sizeof(active_list));
    memset(sample_list, 0, sizeof(sample_list));
    memset(deactivate,  0, sizeof(deactivate));
}
    
void WatchManager::add_watch(const char *id)
{
    //Apply to a free slot
    for(int i=0; i<MAX_WATCH; ++i) {
        if(!active_list[i][0]) {
            strncpy(active_list[i], id, 128);
            new_active = true;
            break;
        }
    }
}

void WatchManager::del_watch(const char *id)
{
    //Queue up the delete
    for(int i=0; i<MAX_WATCH; ++i)
        if(!strcmp(active_list[i], id))
            return (void) (deactivate[i] = true);
}

void WatchManager::tick(void)
{
    new_active = false;

    //Clear deleted slots
    for(int i=0; i<MAX_WATCH; ++i)
        if(deactivate[i])
            memset(active_list[i], 0, 128);
}

bool WatchManager::active(const char *id) const
{
    if(new_active)
        for(int i=0; i<MAX_WATCH; ++i)
            if(!strcmp(active_list[i], id))
                return true;

    return false;
}
    
int WatchManager::samples(const char *id) const
{
    for(int i=0; i<MAX_WATCH; ++i)
        if(!strcmp(active_list[i], id))
            return sample_list[i];
    return 0;
}
    
void WatchManager::satisfy(const char *id, float f)
{
    //printf("trying to satisfy '%s'\n", id);
    if(write_back)
        write_back->write(id, "f", f);
    del_watch(id);
}

