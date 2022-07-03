/*
  ZynAddSubFX - a software synthesizer

  Presets.h - Presets and Clipboard management
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef PRESETS_H
#define PRESETS_H

#include "../globals.h"

namespace zyn {

class PresetsStore;

/**Presets and Clipboard management*/
class Presets
{
    friend class PresetsArray;
    public:
        Presets();
        virtual ~Presets();

        virtual void copy(PresetsStore &ps, const char *name); /**<if name==NULL, the clipboard is used*/
        //virtual void paste(PresetsStore &ps, int npreset); //npreset==0 for clipboard
        virtual bool checkclipboardtype(PresetsStore &ps);
        void deletepreset(PresetsStore &ps, int npreset);

        char type[MAX_PRESETTYPE_SIZE];
        //void setelement(int n);
    protected:
        void setpresettype(const char *type);
    private:
        virtual void add2XML(XMLwrapper& xml)    = 0;
        //virtual void getfromXML(XMLwrapper *xml) = 0;
        //virtual void defaults() = 0;
};

/*
    Location where a "consumer" in zyn is located, where
    consumers are envelopes, LFOs and filters.
    Note that the AD synth global consumers correspond to those of PAD synth
 */
//currently no enum, since this won't work with rPreset
//enum consumer_location_t
//{

#define ad_global_amp 0
#define ad_global_freq 1
#define ad_global_filter 2

#define ad_voice_amp 3
#define ad_voice_freq 4
#define ad_voice_filter 5

#define ad_voice_fm_amp 6
#define ad_voice_fm_freq 7
#define ad_voice_fm_wave 13

#define sub_freq 8
#define sub_filter 9
#define sub_bandwidth 10

#define in_effect 11
#define loc_unspecified 12

//};
using consumer_location_t = int;

enum class consumer_location_type_t
{
    freq, amp, filter, unspecified
};

}

#endif
