/*
  ZynAddSubFX - a software synthesizer

  Part.cpp - Part implementation
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "Part.h"
#include "Microtonal.h"
#include "Util.h"
#include "XMLwrapper.h"
#include "Allocator.h"
#include "../Effects/EffectMgr.h"
#include "../Params/ADnoteParameters.h"
#include "../Params/SUBnoteParameters.h"
#include "../Params/PADnoteParameters.h"
#include "../Synth/Resonance.h"
#include "../Synth/SynthNote.h"
#include "../Synth/ADnote.h"
#include "../Synth/SUBnote.h"
#include "../Synth/PADnote.h"
#include "../Containers/ScratchString.h"
#include "../DSP/FFTwrapper.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <ctime>

#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include <iostream>

namespace zyn {

using rtosc::Ports;
using rtosc::RtData;

#define rObject Part
static const Ports partPorts = {
    rSelf(Part, rEnabledBy(Penabled)),
    rRecurs(kit, 16, "Kit"),//NUM_KIT_ITEMS
    rRecursp(partefx, 3, "Part Effect"),
    rRecur(ctl,       "Controller"),
    rParamZyn(partno, rProp(internal),
              "How many parts are before this in the Master"),
#undef  rChangeCb
#define rChangeCb if(obj->Penabled == false) obj->AllNotesOff();
    rToggle(Penabled, rShort("enable"), rDefaultDepends(partno),
            rPresets(true), rDefault(false), "Part enable"),
#undef rChangeCb
#define rChangeCb
#undef rChangeCb
#define rChangeCb obj->setVolumedB(obj->Volume);
    rParamF(Volume, rShort("Vol"), rDefault(0.0), rUnit(dB),
            rLinear(-40.0, 13.3333), "Part Volume"),
#undef rChangeCb
    {"Pvolume::i", rShort("Vol") rProp(parameter) rLinear(0,127)
        rDefault(96) rDoc("Part Volume"), 0,
        [](const char *m, rtosc::RtData &d) {
            Part *obj = (Part*)d.obj;
        if(rtosc_narguments(m)==0) {
            d.reply(d.loc, "i", (int) roundf(96.0f * obj->Volume / 40.0f + 96.0f));
        } else if(rtosc_narguments(m)==1 && rtosc_type(m,0)=='i') {
            obj->Volume  = obj->volume127TodB(limit<unsigned char>(rtosc_argument(m, 0).i, 0, 127));
            obj->setVolumedB(obj->Volume);
            d.broadcast(d.loc, "i", limit<char>(rtosc_argument(m, 0).i, 0, 127));
        }}},
#define rChangeCb obj->setPpanning(obj->Ppanning);
    rParamZyn(Ppanning, rShort("pan"), rDefault(64), "Set Panning"),
#undef rChangeCb
#define rChangeCb obj->setkeylimit(obj->Pkeylimit);
    rParamI(Pkeylimit, rShort("limit"), rProp(parameter),
    rMap(min,0), rMap(max, POLYPHONY), rDefault(15), "Key limit per part"),
#undef rChangeCb
#define rChangeCb obj->setvoicelimit(obj->Pvoicelimit);
    rParamI(Pvoicelimit, rShort("vlimit"), rProp(parameter),
    rMap(min,0), rMap(max, POLYPHONY), rDefault(0), "Voice limit per part"),
#undef rChangeCb
#define rChangeCb
    rParamZyn(Pminkey, rShort("min"), rDefault(0), "Min Used Key"),
    rParamZyn(Pmaxkey, rShort("max"), rDefault(127), "Max Used Key"),
    rParamZyn(Pkeyshift, rShort("shift"), rDefault(64), "Part keyshift"),
    rParamZyn(Prcvchn, rOptions(ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8, ch9, ch10, ch11, ch12, ch13, ch14, ch15, ch16),
              rPresets(ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8, ch9, ch10, ch11, ch12, ch13, ch14, ch15, ch16),
              "Active MIDI channel"),
    rParamZyn(Pvelsns,   rShort("sense"), rDefault(64), "Velocity sensing"),
    rParamZyn(Pveloffs,  rShort("offset"), rDefault(64),"Velocity offset"),
    rToggle(Pnoteon, rDefault(true), "If the channel accepts note on events"),
    rOption(Pkitmode, rOptions(Off, Multi-Kit, Single-Kit), rDefault(Off),
            "Kit mode/enable\n"
            "Off        - Only the first kit is ever utilized\n"
            "Multi-kit  - Every applicable kit is run for a note\n"
            "Single-kit - The first applicable kit is run for a given note"),
    rToggle(Pdrummode, rDefault(false), "Drum mode enable\n"
            "When drum mode is enabled all keys are mapped to 12tET and legato is disabled"),
    rToggle(Ppolymode, rDefault(true), "Polyphony mode"),
    rToggle(Plegatomode, rDefault(false), "Legato mode"),
    rParamZyn(info.Ptype, rDefault(0), "Class of Instrument"),
    rString(info.Pauthor, MAX_INFO_TEXT_SIZE, rDefault(""),
        "Instrument author"),
    rString(info.Pcomments, MAX_INFO_TEXT_SIZE, rDefault(""),
        "Instrument comments"),
    rString(Pname, PART_MAX_NAME_LEN, rDefault(""), "User specified label"),
    rArrayI(Pefxroute, NUM_PART_EFX,
            rOptions(Next Effect,Part Out,Dry Out),
            ":default\0=[\"Next Effect\"S...]\0",
            "Effect Routing"),
    rArrayT(Pefxbypass, NUM_PART_EFX, rDefault([false...]),
        "If an effect is bypassed"),
    {"captureMin:", rDoc("Capture minimum valid note"), NULL,
        [](const char *, RtData &r)
        {Part *p = (Part*)r.obj; p->Pminkey = p->lastnote;}},
    {"captureMax:", rDoc("Capture maximum valid note"), NULL,
        [](const char *, RtData &r)
        {Part *p = (Part*)r.obj; p->Pmaxkey = p->lastnote;}},
    {"polyType::c:i", rProp(parameter) rOptions(Polyphonic, Monophonic, Legato, Latch)
        rDoc("Synthesis polyphony type\n"
                "Polyphonic - Each note is played independently\n"
                "Monophonic - A single note is played at a time with"
                " envelopes resetting between notes\n"
                "Legato     - A single note is played at a time without"
                " envelopes resetting between notes\n"
                "Latch     - Notes are released when a new one is hit "
                " after key release\n"
            ), NULL,
        [](const char *msg, RtData &d)
        {
            Part *p = (Part*)d.obj;
            auto get_polytype = [&p](){
                int res = 0;
                if(!p->Ppolymode)
                    res = p->Plegatomode ? 2 : 1;
                if(p->Platchmode)
                    res = 3;
                return res;
            };

            if(!rtosc_narguments(msg)) {
                d.reply(d.loc, "i", get_polytype());
                return;
            }

            int i = rtosc_argument(msg, 0).i;
            if(i == 0) {
                p->Ppolymode = 1;
                p->Plegatomode = 0;
                p->Platchmode = 0;
            } else if(i==1) {
                p->Ppolymode = 0;
                p->Plegatomode = 0;
                p->Platchmode = 0;
            } else if(i==2) {
                p->Ppolymode = 0;
                p->Plegatomode = 1;
                p->Platchmode = 0;
            } else {
                p->Ppolymode = 1;
                p->Plegatomode = 0;
                p->Platchmode = 1;
            }
            d.broadcast(d.loc, "i", get_polytype());
        }
    },
    {"clear:", rProp(internal) rDoc("Reset Part To Defaults"), 0,
        [](const char *, RtData &)
        {
            //XXX todo forward this event for middleware to handle
            //Part *p = (Part*)d.obj;
            //p->defaults();
            //char part_loc[128];
            //strcpy(part_loc, d.loc);
            //char *end = strrchr(part_loc, '/');
            //if(end)
            //    end[1] = 0;

            //d.broadcast("/damage", "s", part_loc);
        }},
        {"savexml:", rProp(internal) rDoc("Save Part to the file it has been loaded from"), 0,
        [](const char *, RtData &d)
        {
            Part *p = (Part*)d.obj;
            if (p->loaded_file[0] == '\0') {  // if part was never loaded or saved
                time_t rawtime;     // make a new name from date and time
                char filename[32];
                time (&rawtime);
                const struct tm* timeinfo = localtime (&rawtime);
                strftime (filename,23,"%F_%R.xiz",timeinfo);
                p->saveXML(filename);
                fprintf(stderr, "Part %d saved to %s\n", (p->partno + 1), filename);
            }
            else
            {
                p->saveXML(p->loaded_file);
                fprintf(stderr, "Part %d saved to %s\n", (p->partno + 1), p->loaded_file);
            }
        }},
    //{"kit#16::T:F", "::Enables or disables kit item", 0,
    //    [](const char *m, RtData &d) {
    //        auto loc = d.loc;
    //        Part *p = (Part*)d.obj;
    //        unsigned kitid = -1;
    //        //Note that this event will be captured before transmitted, thus
    //        //reply/broadcast don't matter
    //        for(int i=0; i<NUM_KIT_ITEMS; ++i) {
    //            d.reply("/middleware/oscil", "siisb", loc, kitid, i,
    //                    "oscil", sizeof(void*),
    //                    p->kit[kitid]->adpars->voice[i]->OscilSmp);
    //            d.reply("/middleware/oscil", "siisb", loc, kitid, i, "oscil-mod"
    //                    sizeof(void*),
    //                    p->kit[kitid]->adpars->voice[i]->somethingelse);
    //        }
    //        d.reply("/middleware/pad", "sib", loc, kitid,
    //                sizeof(PADnoteParameters*),
    //                p->kit[kitid]->padpars)
    //    }}
};

#undef  rObject
#define rObject Part::Kit
static const Ports kitPorts = {
    rSelf(Part::Kit, rEnabledBy(Penabled)),
    rRecurp(padpars, rEnabledBy(Ppadenabled), "Padnote parameters"),
    rRecurp(adpars, rEnabledBy(Padenabled), "Adnote parameters"),
    rRecurp(subpars, rEnabledBy(Psubenabled), "Subnote parameters"),
    rToggle(firstkit, rProp(internal), "If this is the part's first kit"),
    rToggle(Penabled, rDefaultDepends(firstkit),
            rPreset(true, true), rPreset(false, false),
            "Kit item enable"),
    rToggle(Pmuted,  rDefault(false), "Kit item mute"),
    rParamZyn(Pminkey, rDefault(0),  "Kit item min key"),
    rParamZyn(Pmaxkey, rDefault(127)  "Kit item max key"),
    rToggle(Padenabled, rDefaultDepends(firstkit),
            rPreset(true, true), rPreset(false, false),
            "ADsynth enable"),
    rToggle(Psubenabled, rDefault(false), "SUBsynth enable"),
    rToggle(Ppadenabled, rDefault(false), "PADsynth enable"),
    rParamZyn(Psendtoparteffect,
            rOptions(FX1, FX2, FX3, Off), rDefault(FX1),
            "Effect Levels"),
    rString(Pname, PART_MAX_NAME_LEN, rDefault(""), "Kit User Specified Label"),
    {"captureMin:", rDoc("Capture minimum valid note"), NULL,
        [](const char *, RtData &r)
        {Part::Kit *p = (Part::Kit*)r.obj; p->Pminkey = p->parent->lastnote;}},
    {"captureMax:", rDoc("Capture maximum valid note"), NULL, [](const char *, RtData &r)
        {Part::Kit *p = (Part::Kit*)r.obj; p->Pmaxkey = p->parent->lastnote;}},
    {"padpars-data:b", rProp(internal) rDoc("Set PADsynth data pointer"), 0,
        [](const char *msg, RtData &d) {
            rObject &o = *(rObject*)d.obj;
            assert(o.padpars == NULL);
            o.padpars = *(decltype(o.padpars)*)rtosc_argument(msg, 0).b.data;
        }},
    {"adpars-data:b", rProp(internal) rDoc("Set ADsynth data pointer"), 0,
        [](const char *msg, RtData &d) {
            rObject &o = *(rObject*)d.obj;
            assert(o.adpars == NULL);
            o.adpars = *(decltype(o.adpars)*)rtosc_argument(msg, 0).b.data;
        }},
    {"subpars-data:b", rProp(internal) rDoc("Set SUBsynth data pointer"), 0,
        [](const char *msg, RtData &d) {
            rObject &o = *(rObject*)d.obj;
            assert(o.subpars == NULL);
            o.subpars = *(decltype(o.subpars)*)rtosc_argument(msg, 0).b.data;
        }},
};

const Ports &Part::Kit::ports = kitPorts;
const Ports &Part::ports = partPorts;

Part::Part(Allocator &alloc, const SYNTH_T &synth_, const AbsTime &time_,
    const int &gzip_compression, const int &interpolation,
    Microtonal *microtonal_, FFTwrapper *fft_, WatchManager *wm_, const char *prefix_)
    :Pdrummode(false),
    Ppolymode(true),
    Plegatomode(false),
    Platchmode(false),
    partoutl(new float[synth_.buffersize]),
    partoutr(new float[synth_.buffersize]),
    ctl(synth_, &time_),
    microtonal(microtonal_),
    fft(fft_),
    wm(wm_),
    memory(alloc),
    synth(synth_),
    time(time_),
    gzip_compression(gzip_compression),
    interpolation(interpolation)
{
    loaded_file[0] = '\0';

    if(prefix_)
        fast_strcpy(prefix, prefix_, sizeof(prefix));
    else
        memset(prefix, 0, sizeof(prefix));

    monomemClear();

    for(int n = 0; n < NUM_KIT_ITEMS; ++n) {
        kit[n].parent  = this;
        kit[n].Pname   = new char [PART_MAX_NAME_LEN];
        kit[n].adpars  = nullptr;
        kit[n].subpars = nullptr;
        kit[n].padpars = nullptr;
    }

    kit[0].adpars  = new ADnoteParameters(synth, fft, &time);

    //Part's Insertion Effects init
    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx) {
        partefx[nefx]    = new EffectMgr(memory, synth, 1, &time);
        Pefxbypass[nefx] = false;
    }
    assert(partefx[0]);

    for(int n = 0; n < NUM_PART_EFX + 1; ++n) {
        partfxinputl[n] = new float [synth.buffersize];
        partfxinputr[n] = new float [synth.buffersize];
    }

    killallnotes = false;
    oldfreq_log2 = -1.0f;

    cleanup();

    Pname = new char[PART_MAX_NAME_LEN];

    lastnote = -1;

    defaults();
    assert(partefx[0]);
}

Part::Kit::Kit(void)
    :parent(nullptr),
     Penabled(false), Pmuted(false),
     Pminkey(0), Pmaxkey(127),
     Pname(nullptr),
     Padenabled(false), Psubenabled(false),
     Ppadenabled(false), Psendtoparteffect(0),
     adpars(nullptr), subpars(nullptr), padpars(nullptr)
{
}

void Part::cloneTraits(Part &p) const
{
#define CLONE(x) p.x = this->x
    CLONE(Penabled);

    p.setVolumedB(this->Volume);
    p.setPpanning(this->Ppanning);

    CLONE(Pminkey);
    CLONE(Pmaxkey);
    CLONE(Pkeyshift);
    CLONE(Prcvchn);

    CLONE(Pvelsns);
    CLONE(Pveloffs);

    CLONE(Pnoteon);
    CLONE(Ppolymode);
    CLONE(Plegatomode);
    CLONE(Pkeylimit);
    CLONE(Pvoicelimit);

    // Controller has a reference, so it can not be re-assigned
    // So, destroy and reconstruct it.
    p.ctl.~Controller(); new (&p.ctl) Controller(this->ctl);
}

void Part::defaults()
{
    Penabled    = 0;
    Pminkey     = 0;
    Pmaxkey     = 127;
    Pnoteon     = 1;
    Ppolymode   = 1;
    Plegatomode = 0;
    setVolumedB(0.0);
    Pkeyshift = 64;
    Prcvchn   = 0;
    setPpanning(64);
    Pvelsns   = 64;
    Pveloffs  = 64;
    Pkeylimit = 15;
    Pvoicelimit = 0;
    defaultsinstrument();
    ctl.defaults();
}

void Part::defaultsinstrument()
{
    ZERO(Pname, PART_MAX_NAME_LEN);

    info.Ptype = 0;
    ZERO(info.Pauthor, MAX_INFO_TEXT_SIZE + 1);
    ZERO(info.Pcomments, MAX_INFO_TEXT_SIZE + 1);

    Pkitmode  = 0;
    Pdrummode = 0;

    for(int n = 0; n < NUM_KIT_ITEMS; ++n) {
        //kit[n].Penabled    = false;
        kit[n].firstkit    = false;
        kit[n].Pmuted      = false;
        kit[n].Pminkey     = 0;
        kit[n].Pmaxkey     = 127;
        kit[n].Padenabled  = false;
        kit[n].Psubenabled = false;
        kit[n].Ppadenabled = false;
        ZERO(kit[n].Pname, PART_MAX_NAME_LEN);
        kit[n].Psendtoparteffect = 0;
        if(n != 0)
            setkititemstatus(n, 0);
    }
    kit[0].firstkit   = true;
    kit[0].Penabled   = 1;
    kit[0].Padenabled = 1;
    kit[0].adpars->defaults();

    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx) {
        partefx[nefx]->defaults();
        Pefxroute[nefx] = 0; //route to next effect
    }
}



/*
 * Cleanup the part
 */
void Part::cleanup(bool final_)
{
    notePool.killAllNotes();
    for(int i = 0; i < synth.buffersize; ++i) {
        partoutl[i] = final_ ? 0.0f : synth.denormalkillbuf[i];
        partoutr[i] = final_ ? 0.0f : synth.denormalkillbuf[i];
    }
    ctl.resetall();
    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx)
        partefx[nefx]->cleanup();
    for(int n = 0; n < NUM_PART_EFX + 1; ++n)
        for(int i = 0; i < synth.buffersize; ++i) {
            partfxinputl[n][i] = final_ ? 0.0f : synth.denormalkillbuf[i];
            partfxinputr[n][i] = final_ ? 0.0f : synth.denormalkillbuf[i];
        }
}

Part::~Part()
{
    cleanup(true);
    for(int n = 0; n < NUM_KIT_ITEMS; ++n) {
        delete kit[n].adpars;
        delete kit[n].subpars;
        delete kit[n].padpars;
        delete [] kit[n].Pname;
    }

    delete [] Pname;
    delete [] partoutl;
    delete [] partoutr;
    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx)
        delete partefx[nefx];
    for(int n = 0; n < NUM_PART_EFX + 1; ++n) {
        delete [] partfxinputl[n];
        delete [] partfxinputr[n];
    }
}

static void assert_kit_sanity(const Part::Kit *kits)
{
    for(int i=0; i<NUM_KIT_ITEMS; ++i) {
        //an enabled kit must have a corresponding parameter object
        assert(!kits[i].Padenabled  || kits[i].adpars);
        assert(!kits[i].Ppadenabled || kits[i].padpars);
        assert(!kits[i].Psubenabled || kits[i].subpars);
    }
}

static int kit_usage(const Part::Kit *kits, int note, int mode)
{
    const bool non_kit   = mode == 0;
    const bool singl_kit = mode == 2;
    int synth_usage = 0;

    for(uint8_t i = 0; i < NUM_KIT_ITEMS; ++i) {
        const auto &item = kits[i];
        if(!non_kit && !item.validNote(note))
            continue;

        synth_usage += item.Padenabled;
        synth_usage += item.Psubenabled;
        synth_usage += item.Ppadenabled;

        //Partial Kit Use
        if(non_kit || (singl_kit && item.active()))
            break;
    }

    return synth_usage;
}

/*
 * Note On Messages
 */
bool Part::NoteOnInternal(note_t note,
                  unsigned char velocity,
                  float note_log2_freq)
{
    //Verify Basic Mode and sanity
    const bool isRunningNote   = notePool.existsRunningNote();
    const bool doingLegato     = isRunningNote && isLegatoMode() &&
                                 lastlegatomodevalid;

    if(!Pnoteon || !inRange(note, Pminkey, Pmaxkey) || notePool.full() ||
            notePool.synthFull(kit_usage(kit, note, Pkitmode)))
        return false;

    verifyKeyMode();
    assert_kit_sanity(kit);

    //Preserve Note Stack
    if(isMonoMode() || isLegatoMode()) {
        monomemPush(note);
        monomem[note].velocity  = velocity;
        monomem[note].note_log2_freq = note_log2_freq;

    } else if(!monomemEmpty())
        monomemClear();

    //Mono/Legato Release old notes
    if(isMonoMode() || (isLegatoMode() && !doingLegato))
        notePool.releasePlayingNotes();

    lastlegatomodevalid = isLegatoMode();

    //Compute Note Parameters
    const float vel          = getVelocity(velocity, Pvelsns, Pveloffs);

    //Portamento
    lastnote = note;

    /* check if first note is played */
    if(oldfreq_log2 < 0.0f)
        oldfreq_log2 = note_log2_freq;

    // For Mono/Legato: Force Portamento Off on first
    // notes. That means it is required that the previous note is
    // still held down or sustained for the Portamento to activate
    // (that's like Legato).
    const bool portamento = (Ppolymode || isRunningNote) &&
        ctl.initportamento(oldfreq_log2, note_log2_freq, doingLegato);

    oldfreq_log2 = note_log2_freq;

    //Adjust Existing Notes
    if(doingLegato) {
        LegatoParams pars = {vel, portamento, note_log2_freq, true, prng()};
        notePool.applyLegato(note, pars);
        return true;
    }

    if(Ppolymode)
        notePool.makeUnsustainable(note);
    
    // in latch mode release latched notes before creating the new one
    if(Platchmode)
        notePool.releaseLatched();

    //Create New Notes
    for(uint8_t i = 0; i < NUM_KIT_ITEMS; ++i) {
        ScratchString pre = prefix;
        auto &item = kit[i];
        if(Pkitmode != 0 && !item.validNote(note))
            continue;

        SynthParams pars{memory, ctl, synth, time, vel,
            portamento, note_log2_freq, false, prng()};
        const int sendto = Pkitmode ? item.sendto() : 0;

        // Enforce voice limit, before we trigger new note
        limit_voices(note);

        try {
            if(item.Padenabled)
                notePool.insertNote(note, sendto,
                        {memory.alloc<ADnote>(kit[i].adpars, pars,
                            wm, (pre+"kit"+i+"/adpars/").c_str), 0, i});
            if(item.Psubenabled)
                notePool.insertNote(note, sendto,
                        {memory.alloc<SUBnote>(kit[i].subpars, pars, wm, (pre+"kit"+i+"/subpars/").c_str), 1, i});
            if(item.Ppadenabled)
                notePool.insertNote(note, sendto,
                        {memory.alloc<PADnote>(kit[i].padpars, pars, interpolation, wm,
                            (pre+"kit"+i+"/padpars/").c_str), 2, i});
        } catch (std::bad_alloc & ba) {
            std::cerr << "dropped new note: " << ba.what() << std::endl;
        }

        //Partial Kit Use
        if(isNonKit() || (isSingleKit() && item.active()))
            break;
    }

    if(isLegatoMode())
        notePool.upgradeToLegato();

    //Enforce the key limit
    setkeylimit(Pkeylimit);
    return true;
}

/*
 * Note Off Messages
 */
void Part::NoteOff(note_t note) //release the key
{
    // This note is released, so we remove it from the list.
    if(!monomemEmpty())
        monomemPop(note);

    for(auto &desc:notePool.activeDesc()) {
        if(desc.note != note || !desc.playing())
            continue;
        // if latch is on we ignore noteoff, but set the state to lateched
        if(Platchmode) {
            notePool.latch(desc);
        } else if(!ctl.sustain.sustain) { //the sustain pedal is not pushed
            if((isMonoMode() || isLegatoMode()) && !monomemEmpty())
                MonoMemRenote();//Play most recent still active note
            else
                notePool.release(desc);
        }
        else {   //the sustain pedal is pushed
            if(desc.canSustain())
                desc.doSustain();
            else {
                notePool.release(desc);
            }
        }
    }
}

void Part::PolyphonicAftertouch(note_t note,
                unsigned char velocity)
{
    if(!Pnoteon || !inRange(note, Pminkey, Pmaxkey) || Pdrummode)
        return;

    /*
     * Don't allow the velocity to reach zero.
     * Keep it alive until note off.
     */
    if(velocity == 0)
        velocity = 1;

    // MonoMem stuff:
    if(!Ppolymode)   // if Poly is off
        monomem[note].velocity = velocity;       // Store this note's velocity.

    const float vel = getVelocity(velocity, Pvelsns, Pveloffs);
    for(auto &d:notePool.activeDesc()) {
        if(d.note == note && d.playing())
            for(auto &s:notePool.activeNotes(d))
                s.note->setVelocity(vel);
    }
}

/*
 * Controllers
 */
void Part::SetController(unsigned int type, int par)
{
    switch(type) {
        case C_pitchwheel:
            ctl.setpitchwheel(par);
            break;
        case C_expression:
            ctl.setexpression(par);
            setVolumedB(Volume); //update the volume
            break;
        case C_portamento:
            ctl.setportamento(par);
            break;
        case C_panning:
            ctl.setpanning(par);
            setPpanning(Ppanning); //update the panning
            break;
        case C_filtercutoff:
            ctl.setfiltercutoff(par);
            break;
        case C_filterq:
            ctl.setfilterq(par);
            break;
        case C_bandwidth:
            ctl.setbandwidth(par);
            break;
        case C_modwheel:
            ctl.setmodwheel(par);
            break;
        case C_fmamp:
            ctl.setfmamp(par);
            break;
        case C_volume:
            ctl.setvolume(par);
            if(ctl.volume.receive != 0)
                setVolumedB(volume127TodB( ctl.volume.volume * 127.0f ) );
            else
                /* FIXME: why do this? */
                setVolumedB(Volume);
            break;
        case C_sustain:
            ctl.setsustain(par);
            if(ctl.sustain.sustain == 0)
                ReleaseSustainedKeys();
            break;
        case C_allsoundsoff:
            AllNotesOff(); //Panic
            break;
        case C_resetallcontrollers:
            ctl.resetall();
            ReleaseSustainedKeys();
            if(ctl.volume.receive != 0)
                setVolumedB(volume127TodB( ctl.volume.volume * 127.0f ) );
            else
                setVolumedB(Volume);
            setPpanning(Ppanning); //update the panning

            for(int item = 0; item < NUM_KIT_ITEMS; ++item) {
                if(kit[item].adpars == NULL)
                    continue;
                kit[item].adpars->GlobalPar.Reson->
                sendcontroller(C_resonance_center, 1.0f);

                kit[item].adpars->GlobalPar.Reson->
                sendcontroller(C_resonance_bandwidth, 1.0f);
            }
            //more update to add here if I add controllers
            break;
        case C_allnotesoff:
            ReleaseAllKeys();
            break;
        case C_resonance_center:
            ctl.setresonancecenter(par);
            for(int item = 0; item < NUM_KIT_ITEMS; ++item) {
                if(kit[item].adpars == NULL)
                    continue;
                kit[item].adpars->GlobalPar.Reson->
                sendcontroller(C_resonance_center,
                               ctl.resonancecenter.relcenter);
            }
            break;
        case C_resonance_bandwidth:
            ctl.setresonancebw(par);
            kit[0].adpars->GlobalPar.Reson->
            sendcontroller(C_resonance_bandwidth, ctl.resonancebandwidth.relbw);
            break;
    }
}

/*
 * Per note controllers.
 */
void Part::SetController(unsigned int type, note_t note, float value,
                         int masterkeyshift)
{
    if(!Pnoteon || !inRange(note, Pminkey, Pmaxkey) || Pdrummode)
        return;

    switch (type) {
    case C_aftertouch:
        PolyphonicAftertouch(note, floorf(value));
        break;
    case C_pitch: {
        if (getNoteLog2Freq(masterkeyshift, value) == false)
            break;

        /* Make sure MonoMem's frequency information is kept up to date */
        if(!Ppolymode)
            monomem[note].note_log2_freq = value;

        for(auto &d:notePool.activeDesc()) {
            if(d.note == note && d.playing())
                for(auto &s:notePool.activeNotes(d))
                    s.note->setPitch(value);
        }
        break;
    }
    case C_filtercutoff:
        for(auto &d:notePool.activeDesc()) {
            if(d.note == note && d.playing())
                for(auto &s:notePool.activeNotes(d))
                    s.note->setFilterCutoff(value);
        }
        break;
    default:
        break;
    }
}

/*
 * Release the sustained keys
 */

void Part::ReleaseSustainedKeys()
{
    // Let's call MonoMemRenote() on some conditions:
    if((isMonoMode() || isLegatoMode()) && !monomemEmpty())
        if(monomemBack() != lastnote) // Sustain controller manipulation would cause repeated same note respawn without this check.
            MonoMemRenote();  // To play most recent still held note.

    for(auto &d:notePool.activeDesc())
        if(d.sustained())
            for(auto &s:notePool.activeNotes(d))
                s.note->releasekey();
}

/*
 * Release all keys
 */

void Part::ReleaseAllKeys()
{
    for(auto &d:notePool.activeDesc())
        if(!d.released())
            for(auto &s:notePool.activeNotes(d))
                s.note->releasekey();
}

// Call NoteOn(...) with the most recent still held key as new note
// (Made for Mono/Legato).
void Part::MonoMemRenote()
{
    note_t mmrtempnote = monomemBack(); // Last list element.
    monomemPop(mmrtempnote); // We remove it, will be added again in NoteOn(...).
    NoteOnInternal(mmrtempnote,
           monomem[mmrtempnote].velocity,
           monomem[mmrtempnote].note_log2_freq);
}

bool Part::getNoteLog2Freq(int masterkeyshift, float &note_log2_freq)
{
    if(Pdrummode) {
        note_log2_freq += log2f(440.0f) - 69.0f / 12.0f;
        return true;
    }
    return microtonal->updatenotefreq_log2(note_log2_freq,
        (int)Pkeyshift - 64 + masterkeyshift);
}

float Part::getVelocity(uint8_t velocity, uint8_t velocity_sense,
        uint8_t velocity_offset) const
{
    //compute sense function
    const float vel = VelF(velocity / 127.0f, velocity_sense);

    //compute the velocity offset
    return limit(vel + (velocity_offset - 64.0f) / 64.0f, 0.0f, 1.0f);
}

void Part::verifyKeyMode(void)
{
    if(Plegatomode && !Pdrummode && Ppolymode) {
            fprintf(stderr,
                "WARNING: Poly & Legato modes are On, that shouldn't happen\n"
                "Disabling Legato mode...\n"
                "(Part.cpp::NoteOn(..))\n");
            Plegatomode = 0;
    }
}


/*
 * Set Part's key limit
 */
void Part::setkeylimit(unsigned char Pkeylimit_)
{
    Pkeylimit = Pkeylimit_;
    int keylimit = Pkeylimit;
    if(keylimit == 0)
        keylimit = POLYPHONY - 5;

    if(notePool.getRunningNotes() >= keylimit)
        notePool.enforceKeyLimit(keylimit);
}

/*
 * Enforce voice limit
 */
void Part::limit_voices(int new_note)
{
    int voicelimit = Pvoicelimit;
    if(voicelimit == 0) /* voice limit disabled */
        return;

    /* If we're called because a new note is imminent, we need to enforce
     * one less note than the limit, so we don't go above the limit when the
     * new note is triggered.
     */
    if (new_note >= 0)
        voicelimit--;

    int running_voices = notePool.getRunningVoices();
    if(running_voices >= voicelimit)
        notePool.enforceVoiceLimit(voicelimit, new_note);
}

/*
 * Set Part's voice limit
 */
void Part::setvoicelimit(unsigned char Pvoicelimit_)
{
    Pvoicelimit = Pvoicelimit_;

    limit_voices(-1);
}

/*
 * Prepare all notes to be turned off
 */
void Part::AllNotesOff()
{
    killallnotes = true;
}

/*
 * Compute Part samples and store them in the partoutl[] and partoutr[]
 */
void Part::ComputePartSmps()
{
    assert(partefx[0]);
    for(unsigned nefx = 0; nefx < NUM_PART_EFX + 1; ++nefx) {
        memset(partfxinputl[nefx], 0, synth.bufferbytes);
        memset(partfxinputr[nefx], 0, synth.bufferbytes);
    }

    for(auto &d:notePool.activeDesc()) {
        d.age++;
        for(auto &s:notePool.activeNotes(d)) {
            float tmpoutr[synth.buffersize];
            float tmpoutl[synth.buffersize];
            auto &note = *s.note;
            note.noteout(&tmpoutl[0], &tmpoutr[0]);

            for(int i = 0; i < synth.buffersize; ++i) { //add the note to part(mix)
                partfxinputl[d.sendto][i] += tmpoutl[i];
                partfxinputr[d.sendto][i] += tmpoutr[i];
            }

            if(note.finished())
                notePool.kill(s);
        }
    }

    //Apply part's effects and mix them
    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx) {
        if(!Pefxbypass[nefx]) {
            partefx[nefx]->out(partfxinputl[nefx], partfxinputr[nefx]);
            if(Pefxroute[nefx] == 2)
                for(int i = 0; i < synth.buffersize; ++i) {
                    partfxinputl[nefx + 1][i] += partefx[nefx]->efxoutl[i];
                    partfxinputr[nefx + 1][i] += partefx[nefx]->efxoutr[i];
                }
        }
        int routeto = ((Pefxroute[nefx] == 0) ? nefx + 1 : NUM_PART_EFX);
        for(int i = 0; i < synth.buffersize; ++i) {
            partfxinputl[routeto][i] += partfxinputl[nefx][i];
            partfxinputr[routeto][i] += partfxinputr[nefx][i];
        }
    }
    for(int i = 0; i < synth.buffersize; ++i) {
        partoutl[i] = partfxinputl[NUM_PART_EFX][i];
        partoutr[i] = partfxinputr[NUM_PART_EFX][i];
    }

    if(killallnotes) {
        for(int i = 0; i < synth.buffersize; ++i) {
            float tmp = (synth.buffersize_f - i) / synth.buffersize_f;
            partoutl[i] *= tmp;
            partoutr[i] *= tmp;
        }
        notePool.killAllNotes();
        monomemClear();
        killallnotes = false;
        for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx)
            partefx[nefx]->cleanup();
    }
    ctl.updateportamento();
}

/*
 * Parameter control
 */

float Part::volume127TodB(unsigned char volume_)
{
    assert( volume_  <= 127 );
    return (volume_ - 96.0f) / 96.0f * 40.0;
}

void Part::setVolumedB(float Volume_)
{
    //Fixes bug with invalid loading
    if(fabs(Volume_ - 50.0f) < 0.001)
        Volume_ = 0.0f;

    Volume_ = limit(Volume_, -40.0f, 13.333f);

    assert(Volume_ < 14.0);
    Volume = Volume_;

    float volume = dB2rap( Volume_ );

    /* printf( "Volume: %f, Expression %f\n", volume, ctl.expression.relvolume ); */

    assert( volume <= dB2rap(14.0f) );

    gain = volume * ctl.expression.relvolume;
}

void Part::setPpanning(char Ppanning_)
{
    Ppanning = Ppanning_;
    panning  = limit(Ppanning / 127.0f + ctl.panning.pan, 0.0f, 1.0f);
}

/*
 * Enable or disable a kit item
 */
void Part::setkititemstatus(unsigned kititem, bool Penabled_)
{
    //nonexistent kit item and the first kit item is always enabled
    if((kititem == 0) || (kititem >= NUM_KIT_ITEMS))
        return;

    Kit &kkit = kit[kititem];

    //no need to update if
    if(kkit.Penabled == Penabled_)
        return;
    kkit.Penabled = Penabled_;

    if(!Penabled_) {
        delete kkit.adpars;
        delete kkit.subpars;
        delete kkit.padpars;
        kkit.adpars  = nullptr;
        kkit.subpars = nullptr;
        kkit.padpars = nullptr;
        kkit.Pname[0] = '\0';

        notePool.killAllNotes();
    }
    else {
        //All parameters must be NULL in this case
        assert(!(kkit.adpars || kkit.subpars || kkit.padpars));
        kkit.adpars  = new ADnoteParameters(synth, fft, &time);
        kkit.subpars = new SUBnoteParameters(&time);
        kkit.padpars = new PADnoteParameters(synth, fft, &time);
    }
}

void Part::add2XMLinstrument(XMLwrapper& xml)
{
    xml.beginbranch("INFO");
    xml.addparstr("name", (char *)Pname);
    xml.addparstr("author", (char *)info.Pauthor);
    xml.addparstr("comments", (char *)info.Pcomments);
    xml.addpar("type", info.Ptype);
    xml.endbranch();


    xml.beginbranch("INSTRUMENT_KIT");
    xml.addpar("kit_mode", Pkitmode);
    xml.addparbool("drum_mode", Pdrummode);

    for(int i = 0; i < NUM_KIT_ITEMS; ++i) {
        xml.beginbranch("INSTRUMENT_KIT_ITEM", i);
        xml.addparbool("enabled", kit[i].Penabled);
        if(kit[i].Penabled != 0) {
            xml.addparstr("name", (char *)kit[i].Pname);

            xml.addparbool("muted", kit[i].Pmuted);
            xml.addpar("min_key", kit[i].Pminkey);
            xml.addpar("max_key", kit[i].Pmaxkey);

            xml.addpar("send_to_instrument_effect", kit[i].Psendtoparteffect);

            xml.addparbool("add_enabled", kit[i].Padenabled);
            if(kit[i].Padenabled && kit[i].adpars) {
                xml.beginbranch("ADD_SYNTH_PARAMETERS");
                kit[i].adpars->add2XML(xml);
                xml.endbranch();
            }

            xml.addparbool("sub_enabled", kit[i].Psubenabled);
            if(kit[i].Psubenabled && kit[i].subpars) {
                xml.beginbranch("SUB_SYNTH_PARAMETERS");
                kit[i].subpars->add2XML(xml);
                xml.endbranch();
            }

            xml.addparbool("pad_enabled", kit[i].Ppadenabled);
            if(kit[i].Ppadenabled && kit[i].padpars) {
                xml.beginbranch("PAD_SYNTH_PARAMETERS");
                kit[i].padpars->add2XML(xml);
                xml.endbranch();
            }
        }
        xml.endbranch();
    }
    xml.endbranch();

    xml.beginbranch("INSTRUMENT_EFFECTS");
    for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx) {
        xml.beginbranch("INSTRUMENT_EFFECT", nefx);
        xml.beginbranch("EFFECT");
        partefx[nefx]->add2XML(xml);
        xml.endbranch();

        xml.addpar("route", Pefxroute[nefx]);
        partefx[nefx]->setdryonly(Pefxroute[nefx] == 2);
        xml.addparbool("bypass", Pefxbypass[nefx]);
        xml.endbranch();
    }
    xml.endbranch();
}

void Part::add2XML(XMLwrapper& xml)
{
    //parameters
    xml.addparbool("enabled", Penabled);
    if((Penabled == 0) && (xml.minimal))
        return;

    xml.addparreal("volume", Volume);
    xml.addpar("panning", Ppanning);

    xml.addpar("min_key", Pminkey);
    xml.addpar("max_key", Pmaxkey);
    xml.addpar("key_shift", Pkeyshift);
    xml.addpar("rcv_chn", Prcvchn);

    xml.addpar("velocity_sensing", Pvelsns);
    xml.addpar("velocity_offset", Pveloffs);

    xml.addparbool("note_on", Pnoteon);
    xml.addparbool("poly_mode", Ppolymode);
    xml.addpar("legato_mode", Plegatomode);
    xml.addpar("key_limit", Pkeylimit);
    xml.addpar("voice_limit", Pvoicelimit);

    xml.beginbranch("INSTRUMENT");
    add2XMLinstrument(xml);
    xml.endbranch();

    xml.beginbranch("CONTROLLER");
    ctl.add2XML(xml);
    xml.endbranch();
}

int Part::saveXML(const char *filename)
{
    XMLwrapper xml;

    xml.beginbranch("INSTRUMENT");
    add2XMLinstrument(xml);
    xml.endbranch();

    int result = xml.saveXMLfile(filename, gzip_compression);
    return result;
}

int Part::loadXMLinstrument(const char *filename)
{
    XMLwrapper xml;
    if(xml.loadXMLfile(filename) < 0) {
        return -1;
    }

    if(xml.enterbranch("INSTRUMENT") == 0)
        return -10;

    // store filename in member variable
    int length = sizeof(loaded_file)-1;
    strncpy(loaded_file, filename, length);
    // set last element to \0 in case filename is too long or not terminated
    loaded_file[length]='\0';

    getfromXMLinstrument(xml);
    xml.exitbranch();

    return 0;
}

void Part::applyparameters(void)
{
    applyparameters([]{return false;});
}

void Part::applyparameters(std::function<bool()> do_abort)
{
    for(int n = 0; n < NUM_KIT_ITEMS; ++n)
        if(kit[n].Ppadenabled && kit[n].padpars)
            kit[n].padpars->applyparameters(do_abort);
}

void Part::initialize_rt(void)
{
    for(int i=0; i<NUM_PART_EFX; ++i)
        partefx[i]->init();
}

void Part::kill_rt(void)
{
    for(int i=0; i<NUM_PART_EFX; ++i)
        partefx[i]->kill();
    notePool.killAllNotes();
}

void Part::monomemPush(note_t note)
{
    for(int i=0; i<256; ++i)
        if(monomemnotes[i]==note)
            return;

    for(int i=254;i>=0; --i)
        monomemnotes[i+1] = monomemnotes[i];
    monomemnotes[0] = note;
}

void Part::monomemPop(note_t note)
{
    int note_pos=-1;
    for(int i=0; i<256; ++i)
        if(monomemnotes[i]==note)
            note_pos = i;
    if(note_pos != -1) {
        for(int i=note_pos; i<256; ++i)
            monomemnotes[i] = monomemnotes[i+1];
        monomemnotes[255] = -1;
    }
}

note_t Part::monomemBack(void) const
{
    return monomemnotes[0];
}

bool Part::monomemEmpty(void) const
{
    return monomemnotes[0] == -1;
}

void Part::monomemClear(void)
{
    for(int i=0; i<256; ++i)
        monomemnotes[i] = -1;
}

void Part::getfromXMLinstrument(XMLwrapper& xml)
{
    if(xml.enterbranch("INFO")) {
        xml.getparstr("name", (char *)Pname, PART_MAX_NAME_LEN);
        xml.getparstr("author", (char *)info.Pauthor, MAX_INFO_TEXT_SIZE);
        xml.getparstr("comments", (char *)info.Pcomments, MAX_INFO_TEXT_SIZE);
        info.Ptype = xml.getpar("type", info.Ptype, 0, 16);

        xml.exitbranch();
    }

    if(xml.enterbranch("INSTRUMENT_KIT")) {
        Pkitmode  = xml.getpar127("kit_mode", Pkitmode);
        Pdrummode = xml.getparbool("drum_mode", Pdrummode);

        setkititemstatus(0, 0);
        for(int i = 0; i < NUM_KIT_ITEMS; ++i) {
            if(xml.enterbranch("INSTRUMENT_KIT_ITEM", i) == 0)
                continue;
            setkititemstatus(i, xml.getparbool("enabled", kit[i].Penabled));
            if(kit[i].Penabled == 0) {
                xml.exitbranch();
                continue;
            }

            xml.getparstr("name", (char *)kit[i].Pname, PART_MAX_NAME_LEN);

            kit[i].Pmuted  = xml.getparbool("muted", kit[i].Pmuted);
            kit[i].Pminkey = xml.getpar127("min_key", kit[i].Pminkey);
            kit[i].Pmaxkey = xml.getpar127("max_key", kit[i].Pmaxkey);

            kit[i].Psendtoparteffect = xml.getpar127(
                "send_to_instrument_effect",
                kit[i].Psendtoparteffect);

            kit[i].Padenabled = xml.getparbool("add_enabled",
                                                kit[i].Padenabled);
            if(xml.enterbranch("ADD_SYNTH_PARAMETERS")) {
                if(!kit[i].adpars)
                    kit[i].adpars = new ADnoteParameters(synth, fft, &time);
                kit[i].adpars->getfromXML(xml);
                xml.exitbranch();
            }

            kit[i].Psubenabled = xml.getparbool("sub_enabled",
                                                 kit[i].Psubenabled);
            if(xml.enterbranch("SUB_SYNTH_PARAMETERS")) {
                if(!kit[i].subpars)
                    kit[i].subpars = new SUBnoteParameters(&time);
                kit[i].subpars->getfromXML(xml);
                xml.exitbranch();
            }

            kit[i].Ppadenabled = xml.getparbool("pad_enabled",
                                                 kit[i].Ppadenabled);
            if(xml.enterbranch("PAD_SYNTH_PARAMETERS")) {
                if(!kit[i].padpars)
                    kit[i].padpars = new PADnoteParameters(synth, fft, &time);
                kit[i].padpars->getfromXML(xml);
                xml.exitbranch();
            }

            xml.exitbranch();
        }

        xml.exitbranch();
    }


    if(xml.enterbranch("INSTRUMENT_EFFECTS")) {
        for(int nefx = 0; nefx < NUM_PART_EFX; ++nefx) {
            if(xml.enterbranch("INSTRUMENT_EFFECT", nefx) == 0)
                continue;
            if(xml.enterbranch("EFFECT")) {
                partefx[nefx]->getfromXML(xml);
                xml.exitbranch();
            }

            Pefxroute[nefx] = xml.getpar("route",
                                          Pefxroute[nefx],
                                          0,
                                          NUM_PART_EFX);
            partefx[nefx]->setdryonly(Pefxroute[nefx] == 2);
            Pefxbypass[nefx] = xml.getparbool("bypass", Pefxbypass[nefx]);
            xml.exitbranch();
        }
        xml.exitbranch();
    }
}

void Part::getfromXML(XMLwrapper& xml)
{
    Penabled = xml.getparbool("enabled", Penabled);
    if (xml.hasparreal("volume")) {
        setVolumedB(xml.getparreal("volume", Volume));
    } else {
        setVolumedB(volume127TodB( xml.getpar127("volume", 96)));
    }
    setPpanning(xml.getpar127("panning", Ppanning));

    Pminkey   = xml.getpar127("min_key", Pminkey);
    Pmaxkey   = xml.getpar127("max_key", Pmaxkey);
    Pkeyshift = xml.getpar127("key_shift", Pkeyshift);
    Prcvchn   = xml.getpar127("rcv_chn", Prcvchn);

    Pvelsns  = xml.getpar127("velocity_sensing", Pvelsns);
    Pveloffs = xml.getpar127("velocity_offset", Pveloffs);

    Pnoteon     = xml.getparbool("note_on", Pnoteon);
    Ppolymode   = xml.getparbool("poly_mode", Ppolymode);
    Plegatomode = xml.getparbool("legato_mode", Plegatomode); //older versions
    if(!Plegatomode)
        Plegatomode = xml.getpar127("legato_mode", Plegatomode);
    Pkeylimit = xml.getpar127("key_limit", Pkeylimit);
    Pvoicelimit = xml.getpar127("voice_limit", Pvoicelimit);


    if(xml.enterbranch("INSTRUMENT")) {
        getfromXMLinstrument(xml);
        xml.exitbranch();
    }

    if(xml.enterbranch("CONTROLLER")) {
        ctl.getfromXML(xml);
        xml.exitbranch();
    }
}

bool Part::Kit::active(void) const
{
    return Padenabled || Psubenabled || Ppadenabled;
}

uint8_t Part::Kit::sendto(void) const
{
    return limit((int)Psendtoparteffect, 0, NUM_PART_EFX);

}

bool Part::Kit::validNote(char note) const
{
    return !Pmuted && inRange((uint8_t)note, Pminkey, Pmaxkey);
}

}
