/*
  ZynAddSubFX - a software synthesizer

  NotePool.cpp - Pool of Synthesizer Engines And Note Instances
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "NotePool.h"
#include "../Misc/Allocator.h"
#include "../Synth/Portamento.h"
#include "../Synth/SynthNote.h"
#include <cstring>
#include <cassert>
#include <iostream>

#define SUSTAIN_BIT 0x08
#define NOTE_MASK   0x07

namespace zyn {

enum NoteStatus {
    KEY_OFF                    = 0x00,
    KEY_PLAYING                = 0x01,
    KEY_RELEASED_AND_SUSTAINED = 0x02,
    KEY_RELEASED               = 0x03,
    KEY_ENTOMBED               = 0x04,
    KEY_LATCHED                = 0x05
};

const char *getStatus(int status)
{
    switch((enum NoteStatus)(status&NOTE_MASK))
    {
        case KEY_OFF:                     return "OFF ";
        case KEY_PLAYING:                 return "PLAY";
        case KEY_RELEASED_AND_SUSTAINED:  return "SUST";
        case KEY_RELEASED:                return "RELA";
        case KEY_ENTOMBED:                return "TOMB";
        case KEY_LATCHED:                 return "LTCH";
        default:                          return "INVD";
    }
}

NotePool::NotePool(void)
    :needs_cleaning(0)
{
    memset(ndesc, 0, sizeof(ndesc));
    memset(sdesc, 0, sizeof(sdesc));
}

bool NotePool::NoteDescriptor::playing(void) const
{
    return (status&NOTE_MASK) == KEY_PLAYING;
}

bool NotePool::NoteDescriptor::latched(void) const
{
    return (status&NOTE_MASK) == KEY_LATCHED;
}

bool NotePool::NoteDescriptor::sustained(void) const
{
    return (status&NOTE_MASK) == KEY_RELEASED_AND_SUSTAINED;
}

bool NotePool::NoteDescriptor::released(void) const
{
    return (status&NOTE_MASK) == KEY_RELEASED;
}

bool NotePool::NoteDescriptor::entombed(void) const
{
    return (status&NOTE_MASK) == KEY_ENTOMBED;
}

// Notes that are no longer playing, for whatever reason.
bool NotePool::NoteDescriptor::dying(void) const
{
    return (status&NOTE_MASK) == KEY_ENTOMBED ||
           (status&NOTE_MASK) == KEY_RELEASED;
}

bool NotePool::NoteDescriptor::off(void) const
{
    return (status&NOTE_MASK) == KEY_OFF;
}

void NotePool::NoteDescriptor::setStatus(uint8_t s)
{
    status &= ~NOTE_MASK;
    status |= (NOTE_MASK&s);
}

void NotePool::NoteDescriptor::doSustain(void)
{
    setStatus(KEY_RELEASED_AND_SUSTAINED);
}

bool NotePool::NoteDescriptor::canSustain(void) const
{
    return !(status & SUSTAIN_BIT);
}

void NotePool::NoteDescriptor::makeUnsustainable(void)
{
    status |= SUSTAIN_BIT;
}

NotePool::activeNotesIter NotePool::activeNotes(NoteDescriptor &n)
{
    const int off_d1 = &n-ndesc;
    int off_d2 = 0;
    assert(off_d1 <= POLYPHONY);
    for(int i=0; i<off_d1; ++i)
        off_d2 += ndesc[i].size;
    return NotePool::activeNotesIter{sdesc+off_d2,sdesc+off_d2+n.size};
}

bool NotePool::NoteDescriptor::operator==(NoteDescriptor nd)
{
    return age == nd.age && note == nd.note && sendto == nd.sendto && size == nd.size && status == nd.status;
}

//return either the first unused descriptor or the last valid descriptor which
//matches note/sendto
static int getMergeableDescriptor(note_t note, uint8_t sendto, bool legato,
        NotePool::NoteDescriptor *ndesc)
{
    int desc_id;

    for(desc_id = 0; desc_id != POLYPHONY; ++desc_id) {
        if(ndesc[desc_id].off())
            break;
    }

    if(desc_id != 0) {
        auto &nd = ndesc[desc_id-1];
        if(nd.age == 0 && nd.note == note && nd.sendto == sendto
                && nd.playing() && nd.legatoMirror == legato && nd.canSustain())
            return desc_id-1;
    }

    //Out of free descriptors
    if(desc_id == POLYPHONY || !ndesc[desc_id].off()) {
        return -1;
    }

    return desc_id;
}

NotePool::activeDescIter NotePool::activeDesc(void)
{
    cleanup();
    return activeDescIter{*this};
}

NotePool::constActiveDescIter NotePool::activeDesc(void) const
{
    const_cast<NotePool*>(this)->cleanup();
    return constActiveDescIter{*this};
}

int NotePool::usedNoteDesc(void) const
{
    if(needs_cleaning)
        const_cast<NotePool*>(this)->cleanup();

    int cnt = 0;
    for(int i=0; i<POLYPHONY; ++i)
        cnt += (ndesc[i].size != 0);
    return cnt;
}

int NotePool::usedSynthDesc(void) const
{
    if(needs_cleaning)
        const_cast<NotePool*>(this)->cleanup();

    int cnt = 0;
    for(int i=0; i<POLYPHONY*EXPECTED_USAGE; ++i)
        cnt += (bool)sdesc[i].note;
    return cnt;
}

void NotePool::insertNote(note_t note, uint8_t sendto, SynthDescriptor desc, PortamentoRealtime *portamento_realtime, bool legato)
{
    //Get first free note descriptor
    int desc_id = getMergeableDescriptor(note, sendto, legato, ndesc);
    int sdesc_id = 0;
    if(desc_id < 0)
        goto error;

    //Get first free synth descriptor
    while(1) {
        if (sdesc_id == POLYPHONY*EXPECTED_USAGE)
                goto error;
        if (sdesc[sdesc_id].note == 0)
                break;
        sdesc_id++;
    }

    ndesc[desc_id].note                = note;
    ndesc[desc_id].sendto              = sendto;
    ndesc[desc_id].size               += 1;
    ndesc[desc_id].status              = KEY_PLAYING;
    ndesc[desc_id].legatoMirror        = legato;
    ndesc[desc_id].portamentoRealtime  = portamento_realtime;

    sdesc[sdesc_id] = desc;
    return;
error:
    //Avoid leaking note
    desc.note->memory.dealloc(desc.note);
    //Let caller handle failure
    throw std::bad_alloc();
};

void NotePool::upgradeToLegato(void)
{
    for(auto &d:activeDesc())
        if(d.playing())
            for(auto &s:activeNotes(d))
                insertLegatoNote(d, s);
}

void NotePool::insertLegatoNote(NoteDescriptor desc, SynthDescriptor sdesc)
{
    assert(sdesc.note);
    try {
        sdesc.note = sdesc.note->cloneLegato();
        // No portamentoRealtime for the legatoMirror descriptor
        insertNote(desc.note, desc.sendto, sdesc, NULL, true);
    } catch (std::bad_alloc &ba) {
        std::cerr << "failed to insert legato note: " << ba.what() << std::endl;
    }
};

//There should only be one pair of notes which are still playing.
//Note however that there can be releasing legato notes already in the
//list when we get called, so need to handle that.
void NotePool::applyLegato(note_t note, const LegatoParams &par, PortamentoRealtime *portamento_realtime)
{
    for(auto &desc:activeDesc()) {
        //Currently, there can actually be more than one legato pair, while a
        //previous legato pair is releasing and a new one is started, and we
        //don't want to change anything about notes which are releasing.
        if (desc.dying())
            continue;
        desc.note = note;
        // Only set portamentoRealtime for the primary of the two note
        // descriptors in legato mode, or we'll get two note descriptors
        // with the same realtime pointer, causing double updateportamento,
        // and deallocation crashes.
        if (!desc.legatoMirror) {
            //If realtime is already set, we mustn't set it to NULL or we'll
            //leak the old portamento.
            if (portamento_realtime)
                desc.portamentoRealtime = portamento_realtime;
        }
        for(auto &synth:activeNotes(desc))
            try {
                synth.note->legatonote(par);
            } catch (std::bad_alloc& ba) {
                std::cerr << "failed to create legato note: " << ba.what() << std::endl;
            }
    }
}

void NotePool::makeUnsustainable(note_t note)
{
    for(auto &desc:activeDesc()) {
        if(desc.note == note) {
            desc.makeUnsustainable();
            if(desc.sustained())
                release(desc);
        }
    }
}

bool NotePool::full(void) const
{
    for(int i=0; i<POLYPHONY; ++i)
        if(ndesc[i].off())
            return false;
    return true;
}

bool NotePool::synthFull(int sdesc_count) const
{
    int actually_free=sizeof(sdesc)/sizeof(sdesc[0]);
    for(const auto &desc:activeDesc()) {
        actually_free -= desc.size;
    }
    return actually_free < sdesc_count;
}

//Note that isn't KEY_PLAYING or KEY_RELEASED_AND_SUSTAINED
bool NotePool::existsRunningNote(void) const
{
    //printf("running note # =%d\n", getRunningNotes());
    return getRunningNotes();
}

int NotePool::getRunningNotes(void) const
{
    bool running[256] = {};
    int running_count = 0;

    for(auto &desc:activeDesc()) {
        if(desc.playing() == false && desc.sustained() == false && desc.latched() == false)
            continue;
        if(running[desc.note] != false)
            continue;
        running[desc.note] = true;
        running_count++;
    }
    return running_count;
}

void NotePool::enforceKeyLimit(int limit)
{
    int notes_to_kill = getRunningNotes() - limit;
    if(notes_to_kill <= 0)
        return;

    NoteDescriptor *to_kill = NULL;
    unsigned oldest = 0;
    for(auto &nd : activeDesc()) {
        if(to_kill == NULL) {
            //There must be something to kill
            oldest  = nd.age;
            to_kill = &nd;
        } else if(to_kill->dying() && nd.playing()) {
            //Prefer to kill off a running note
            oldest = nd.age;
            to_kill = &nd;
        } else if(nd.age > oldest && !(to_kill->playing() && nd.dying())) {
            //Get an older note when it doesn't move from running to
            //released (or entombed)
            oldest = nd.age;
            to_kill = &nd;
        }
    }

    if(to_kill) {
        auto &tk = *to_kill;
        if(tk.dying() || tk.sustained())
            kill(*to_kill);
        else
            entomb(*to_kill);
    }
}

int NotePool::getRunningVoices(void) const
{
    int running_count = 0;

    for(auto &desc:activeDesc()) {
        // We don't count entombed voices as they will soon be dropped
        if (desc.entombed())
            continue;
        running_count++;
    }

    return running_count;
}

// Silence one voice, trying to the select the one that will be the least
// intrusive, preferably preferred_note if possible..
void NotePool::limitVoice(int preferred_note)
{
    NoteDescriptor *oldest_released = NULL;
    NoteDescriptor *oldest_released_samenote = NULL;
    NoteDescriptor *oldest_sustained = NULL;
    NoteDescriptor *oldest_sustained_samenote = NULL;
    NoteDescriptor *oldest_latched = NULL;
    NoteDescriptor *oldest_latched_samenote = NULL;
    NoteDescriptor *oldest_playing = NULL;
    NoteDescriptor *oldest_playing_samenote = NULL;

    for(auto &nd : activeDesc()) {
        // printf("Scanning %d (%s (%d), age %u)\n", nd.note, getStatus(nd.status), nd.status, nd.age);
        if (nd.released()) {
            if (!oldest_released || nd.age > oldest_released->age)
                oldest_released = &nd;
            if (nd.note == preferred_note &&
                (!oldest_released_samenote || oldest_released_samenote->age))
                oldest_released_samenote = &nd;
        } else if (nd.sustained()) {
            if (!oldest_sustained || nd.age > oldest_sustained->age)
                oldest_sustained = &nd;
            if (nd.note == preferred_note &&
                (!oldest_sustained_samenote || oldest_sustained_samenote->age))
                oldest_sustained_samenote = &nd;
        } else if (nd.latched()) {
            if (!oldest_latched || nd.age > oldest_latched->age)
                oldest_latched = &nd;
            if (nd.note == preferred_note &&
                (!oldest_latched_samenote || oldest_latched_samenote->age))
                oldest_latched_samenote = &nd;
        } else if (nd.playing()) {
            if (!oldest_playing || nd.age > oldest_playing->age)
                oldest_playing = &nd;
            if (nd.note == preferred_note &&
                (!oldest_playing_samenote || oldest_playing_samenote->age))
                oldest_playing_samenote = &nd;
        }
    }

    // Prioritize which note to kill: if a released note exists, take that,
    // otherwise sustained, latched or playing, in that order.
    // Within each category, favour a voice that is already playing the
    // same note, which minimizes stealing notes that are still playing
    // something significant, especially when there are a lot of repeated
    // notes being played.
    // If we don't have anything to kill, there's a logical error somewhere,
    // but we can't do anything about it here so just silently return.

    NoteDescriptor *to_kill = NULL;

    if (oldest_released_samenote)
        to_kill = oldest_released_samenote;
    else if (oldest_released)
        to_kill = oldest_released;
    else if (oldest_sustained_samenote)
        to_kill = oldest_sustained_samenote;
    else if (oldest_sustained)
        to_kill = oldest_sustained;
    else if (oldest_latched_samenote)
        to_kill = oldest_latched_samenote;
    else if (oldest_latched)
        to_kill = oldest_latched;
    else if (oldest_playing_samenote)
        to_kill = oldest_playing_samenote;
    else if (oldest_playing)
        to_kill = oldest_playing;

    if (to_kill) {
        // printf("Will kill %d (age %d)\n", to_kill->note, to_kill->age);
        entomb(*to_kill);
    }
}

void NotePool::enforceVoiceLimit(int limit, int preferred_note)
{
    int notes_to_kill = getRunningVoices() - limit;

    while (notes_to_kill-- > 0)
        limitVoice(preferred_note);
}


void NotePool::releasePlayingNotes(void)
{
    for(auto &d:activeDesc()) {
        if(d.playing() || d.sustained() || d.latched()) {
            d.setStatus(KEY_RELEASED);
            for(auto s:activeNotes(d))
                s.note->releasekey();
        }
    }
}

void NotePool::releaseSustainingNotes(void)
{
    for(auto &d:activeDesc()) {
        if(d.sustained()) {
            d.setStatus(KEY_RELEASED);
            for(auto s:activeNotes(d))
                s.note->releasekey();
        }
    }
}

void NotePool::release(NoteDescriptor &d)
{
    d.setStatus(KEY_RELEASED);
    for(auto s:activeNotes(d))
        s.note->releasekey();
}

void NotePool::latch(NoteDescriptor &d)
{
    d.setStatus(KEY_LATCHED);
}

void NotePool::releaseLatched()
{
    for(auto &desc:activeDesc())
        if(desc.latched())
            for(auto s:activeNotes(desc))
                s.note->releasekey();
}

void NotePool::killAllNotes(void)
{
    for(auto &d:activeDesc())
        kill(d);
}

void NotePool::killNote(note_t note)
{
    for(auto &d:activeDesc()) {
        if(d.note == note)
            kill(d);
    }
}

void NotePool::kill(NoteDescriptor &d)
{
    d.setStatus(KEY_OFF);
    for(auto &s:activeNotes(d))
        kill(s);
    if (d.portamentoRealtime)
        d.portamentoRealtime->memory.dealloc(d.portamentoRealtime);
}

void NotePool::kill(SynthDescriptor &s)
{
    //printf("Kill synth...\n");
    s.note->memory.dealloc(s.note);
    needs_cleaning = true;
}

void NotePool::entomb(NoteDescriptor &d)
{
    d.setStatus(KEY_ENTOMBED);
    for(auto &s:activeNotes(d))
        s.note->entomb();
}

void NotePool::cleanup(void)
{
    if(!needs_cleaning)
        return;
    needs_cleaning = false;
    int new_length[POLYPHONY] = {};
    int cur_length[POLYPHONY] = {};
    //printf("Cleanup Start\n");
    //dump();

    //Identify the current length of all segments
    //and the lengths discarding invalid entries

    int last_valid_desc = 0;
    for(int i=0; i<POLYPHONY; ++i)
        if(!ndesc[i].off())
            last_valid_desc = i;

    //Find the real numbers of allocated notes
    {
        int cum_old = 0;

        for(int i=0; i<=last_valid_desc; ++i) {
            cur_length[i] = ndesc[i].size;
            for(int j=0; j<ndesc[i].size; ++j)
                new_length[i] += (bool)sdesc[cum_old++].note;
        }
    }


    //Move the note descriptors
    {
        int cum_new = 0;
        for(int i=0; i<=last_valid_desc; ++i) {
            ndesc[i].size = new_length[i];
            if(new_length[i] != 0)
                ndesc[cum_new++] = ndesc[i];
            else {
                ndesc[i].setStatus(KEY_OFF);
                if (ndesc[i].portamentoRealtime)
                    ndesc[i].portamentoRealtime->memory.dealloc(ndesc[i].portamentoRealtime);
            }
        }
        memset(ndesc+cum_new, 0, sizeof(*ndesc)*(POLYPHONY-cum_new));
    }

    //Move the synth descriptors
    {
        int total_notes=0;
        for(int i=0; i<=last_valid_desc; ++i)
            total_notes+=cur_length[i];

        int cum_new = 0;
        for(int i=0; i<total_notes; ++i)
            if(sdesc[i].note)
                sdesc[cum_new++] = sdesc[i];
        memset(sdesc+cum_new, 0, sizeof(*sdesc)*(POLYPHONY*EXPECTED_USAGE-cum_new));
    }
    //printf("Cleanup Done\n");
    //dump();
}

void NotePool::dump(void)
{
    printf("NotePool::dump<\n");
    const char *format =
        "    Note %d:%d age(%d) note(%d) sendto(%d) status(%s) legato(%d) type(%d) kit(%d) ptr(%p)\n";
    int note_id=0;
    int descriptor_id=0;
    for(auto &d:activeDesc()) {
        descriptor_id += 1;
        for(auto &s:activeNotes(d)) {
            note_id += 1;
            printf(format,
                    note_id, descriptor_id,
                    d.age, d.note, d.sendto,
                    getStatus(d.status), d.legatoMirror, s.type, s.kit, s.note);
        }
    }
    printf(">NotePool::dump\n");
}

}
