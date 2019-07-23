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
#include "../Misc/Util.h"
#include <cstring>
#include <rtosc/thread-link.h>

namespace zyn {

WatchPoint::WatchPoint(WatchManager *ref, const char *prefix, const char *id)
    :active(false), samples_left(0), reference(ref)
{
    identity[0] = 0;
    if(prefix)
        fast_strcpy(identity, prefix, sizeof(identity));
    if(id)
        strncat(identity, id, sizeof(identity));
}

bool WatchPoint::is_active(void)
{
    //Either the watchpoint is already active or the watchpoint manager has
    //received another activation this frame
    if(active)
        return true;

    if(reference && reference->active(identity)) {
        active       = true;
        samples_left = 1;
        return true;
    }

    return false;
}

FloatWatchPoint::FloatWatchPoint(WatchManager *ref, const char *prefix, const char *id)
    :WatchPoint(ref, prefix, id)
{}

VecWatchPoint::VecWatchPoint(WatchManager *ref, const char *prefix, const char *id)
    :WatchPoint(ref, prefix, id)
{}

WatchManager::WatchManager(thrlnk *link)
    :write_back(link), new_active(false)
{
    memset(active_list, 0, sizeof(active_list));
    memset(sample_list, 0, sizeof(sample_list));
    memset(data_list,   0, sizeof(data_list));
    memset(deactivate,  0, sizeof(deactivate));
    memset(prebuffer,  0, sizeof(prebuffer));
    memset(trigger,  0, sizeof(trigger));
    memset(prebuffer_done,  0, sizeof(prebuffer_done));

}

void WatchManager::add_watch(const char *id)
{
    //Don't add duplicate watchs
    for(int i=0; i<MAX_WATCH; ++i)
        if(!strcmp(active_list[i], id))
            return;

    //Apply to a free slot
    for(int i=0; i<MAX_WATCH; ++i) {
        if(!active_list[i][0]) {
            fast_strcpy(active_list[i], id, MAX_WATCH_PATH);
            new_active = true;
            sample_list[i] = 0;
            //printf("\n added watchpoint ID %s\n",id);
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
    //Try to send out any vector stuff
    for(int i=0; i<MAX_WATCH; ++i) {
        int framesize = 2;
        if(strstr(active_list[i], "noteout") != NULL)
            framesize = MAX_SAMPLE-1;
        //printf("\n framesize: %d  \n",framesize);
        if(sample_list[i] >= framesize-1) {
            char        arg_types[MAX_SAMPLE+1] = {0};
            rtosc_arg_t arg_val[MAX_SAMPLE];
            for(int j=0; j<sample_list[i]; ++j) {
                arg_types[j] = 'f';
                arg_val[j].f = data_list[i][j];
            }

            write_back->writeArray(active_list[i], arg_types, arg_val);
            deactivate[i] = true;
        }
    }

    //Cleanup internal data
    new_active = false;

    //Clear deleted slots
    for(int i=0; i<MAX_WATCH; ++i) {
        if(deactivate[i]) {
            //printf("\ndelete id : %s\n",active_list[i]);
            memset(active_list[i], 0, MAX_SAMPLE);
            sample_list[i] = 0;
            memset(data_list[i], 0, sizeof(float)*MAX_SAMPLE);
            memset(prebuffer[i], 0, sizeof(float)*MAX_SAMPLE);
            deactivate[i]  = false;
            trigger[i] = false;
            prebuffer_done[i] = false;

        }
    }

}

bool WatchManager::active(const char *id) const
{
    assert(this);
    assert(id);
    if(new_active || true)
        for(int i=0; i<MAX_WATCH; ++i)
            if(!strcmp(active_list[i], id))
                return true;

    return false;
}

bool WatchManager::trigger_active(const char *id) const
{
    for(int i=0; i<MAX_WATCH; ++i)
        if(!strcmp(active_list[i], id))
            return trigger[i];
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

void WatchManager::satisfy(const char *id, float *f, int n)
{   
    int selected = -1;    
    for(int i=0; i<MAX_WATCH; ++i)
        if(!strcmp(active_list[i], id))
            selected = i;

    if(selected == -1)
        return;

    // printf("\npath : %s  \n", id);

    // if (!strcmp(id,"/part0/kit0/subpars/noteout"))
    //     printf("\n matched: %s\n", id);

    int space = MAX_SAMPLE - sample_list[selected];
    if(selected == 1)
    printf("\nspace:%d\n",space);

    for(int i = 0; i < n; ++i){
        prebuffer[selected][i] = f[i];
    }

    if(space >= n)
        space = n;

    if(n == 2)
        trigger[selected] = true;
    if(selected == 1)
    printf("\nspace:%d\n",space);
    int kfd = 0;
    //FIXME buffer overflow
    if(space){
        for(int i=0; i<space; ++i){
            if(!trigger[selected]){
                if(i == 0)
                    i++;
                if (f[i-1] <= 0 && f[i] > 0){
                    trigger[selected] = true;
                    for(int k=0; k<MAX_WATCH; ++k){
                        if(selected != k && !trigger[k]){
                            char tmp[128];
                            char tmp1[128];
                            strcpy(tmp, active_list[selected]);
                            strcpy(tmp1, active_list[k]);
                            if(strlen(active_list[k]) < strlen(active_list[selected]))
                                tmp[strlen(tmp)-1] =0;
                            else if (strlen(active_list[k]) > strlen(active_list[selected]))
                                tmp1[strlen(tmp1)-1] =0;
                            if(!strcmp(tmp1,tmp)){
                                trigger[k] = true;
                                int space_k = MAX_SAMPLE - sample_list[k];

                                if(space_k >= n)
                                    space_k = n;
                                printf("/n putting prebuffer size of %d into %s watchpoint /n",space_k - i,active_list[k]);
                                
                                for(int j = i; j < space_k ; ++j){
                                    data_list[k][sample_list[k]] = prebuffer[k][j];
                                    sample_list[k]++;
                                }
                                prebuffer_done[k] = true;
                                printf("\n t Trigger for %s happen at sample %d \n",active_list[k],sample_list[k] );
                                
                            }
                        }
                    }
                }
            }
            if(trigger[selected] && !prebuffer_done[selected]){
                if(selected == 0){
                printf("\n id : %s, sample_list[select] : %d  , index : %d \n",active_list[selected],sample_list[selected],selected);
                printf("\n value f[i] : %f length of input: %d , current iteration: %d\n",f[i],n,i);}
                data_list[selected][sample_list[selected]] = f[i];
                sample_list[selected]++;
            }

            if(kfd == 0)
            printf("\n Trigger for %s happen at sample %d \n",active_list[selected],sample_list[selected] );
            kfd =1;
        }
        if(prebuffer_done[selected])
            prebuffer_done[selected] = false;
    }
}
}
