/*
  ZynAddSubFX - a software synthesizer

  KitTest.h - Test For Note Allocation Under Kits
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "test-suite.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include "../Misc/Time.h"
#include "../Misc/Allocator.h"
#include "../DSP/FFTwrapper.h"
#include "../Misc/Microtonal.h"
#define private public
#define protected public
#include "../Synth/SynthNote.h"
#include "../Misc/Part.h"
#include "../globals.h"

using namespace std;
using namespace zyn;

SYNTH_T *synth;
int dummy=0;

#define SUSTAIN_BIT 0x08
enum PrivateNoteStatus {
    KEY_OFF                    = 0x00,
    KEY_PLAYING                = 0x01,
    KEY_RELEASED_AND_SUSTAINED = 0x02,
    KEY_RELEASED               = 0x03,
    KEY_ENTOMBED               = 0x04
};


class KitTest
{
    private:
        Alloc      alloc;
        FFTwrapper fft;
        Microtonal microtonal;
        Part *part;
        AbsTime *time;
        float *outL, *outR;
    public:
        KitTest()
            :fft(512), microtonal(dummy)
        {}
        void setUp() {
            synth = new SYNTH_T;
            time  = new AbsTime(*synth);
            outL  = new float[synth->buffersize];
            outR = new float[synth->buffersize];
            memset(outL, 0, synth->bufferbytes);
            memset(outR, 0, synth->bufferbytes);


            part = new Part(alloc, *synth, *time, dummy, dummy, &microtonal, &fft);
        }

        //Standard poly mode with sustain
        void testSustainCase1() {
            //enable sustain
            part->ctl.setsustain(127);

            part->NoteOn(64, 127, 0);
            part->NoteOn(64, 127, 0);
            part->NoteOff(64);

            //first note has moved to release state
            //second note has moved to sustain state

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[0],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=64,
                    .sendto=0,
                    .size=1,
                    .status=KEY_RELEASED|SUSTAIN_BIT,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[1],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=64,
                    .sendto=0,
                    .size=1,
                    .status=KEY_RELEASED_AND_SUSTAINED,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[2],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=0,
                    .sendto=0,
                    .size=0,
                    .status=0,
                    .legatoMirror=false}));
        }

        void testSustainCase2() {
            //enable sustain
            part->ctl.setsustain(127);

            part->NoteOn(64, 127, 0);
            part->NoteOff(64);
            part->NoteOn(64, 127, 0);

            //first note has moved to release state
            //second note has stayed in playing state

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[0],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=64,
                    .sendto=0,
                    .size=1,
                    .status=KEY_RELEASED|SUSTAIN_BIT,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[1],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=64,
                    .sendto=0,
                    .size=1,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[2],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=0,
                    .sendto=0,
                    .size=0,
                    .status=0,
                    .legatoMirror=false}));
        }

        void testMonoSustain() {
            //enable sustain
            part->ctl.setsustain(127);
            part->Ppolymode   = false;

            part->NoteOn(64, 127, 0);
            part->NoteOff(64);
            part->NoteOn(65, 127, 0);

            part->notePool.dump();

            //first note has moved to release state
            //second note has stayed in playing state

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[0],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=64,
                    .sendto=0,
                    .size=1,
                    .status=KEY_RELEASED,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[1],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=65,
                    .sendto=0,
                    .size=1,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[2],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=0,
                    .sendto=0,
                    .size=0,
                    .status=0,
                    .legatoMirror=false}));
        }

        //Enumerate cases of:
        //Legato = {disabled,enabled}
        //Mono   = {disabled, enabled}
        //Kit    = {off, normal, single}
        //ignore legato=enabled, mono=enabled

        //No Kit
        void testNoKitNoLegatoNoMono() {
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[0],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=64,
                    .sendto=0,
                    .size=1,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[1],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=65,
                    .sendto=0,
                    .size=1,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[2],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=0,
                    .sendto=0,
                    .size=0,
                    .status=0,
                    .legatoMirror=false}));
        }

        void testNoKitYesLegatoNoMono() {
            part->Ppolymode   = false;
            part->Plegatomode = true;
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[0],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=65,
                    .sendto=0,
                    .size=1,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[1],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=65,
                    .sendto=0,
                    .size=1,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[2],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=0,
                    .sendto=0,
                    .size=0,
                    .status=0,
                    .legatoMirror=false}));

            TS_NON_NULL(part->notePool.sdesc[0].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].kit,  0);

            TS_NON_NULL(part->notePool.sdesc[1].note);
            if(part->notePool.sdesc[1].note)
                TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].note->legato.silent, true);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].kit,  0);
        }

        void testNoKitNoLegatoYesMono() {
            part->Ppolymode   = false;
            part->Plegatomode = false;
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);


            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[0],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=64,
                    .sendto=0,
                    .size=1,
                    .status=KEY_RELEASED,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[1],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=65,
                    .sendto=0,
                    .size=1,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[2],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=0,
                    .sendto=0,
                    .size=0,
                    .status=0,
                    .legatoMirror=false}));

            TS_NON_NULL(part->notePool.sdesc[0].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].kit,  0);

            TS_NON_NULL(part->notePool.sdesc[1].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].kit,  0);
        }

        //Normal Kit
        //Three patches that overlap give an overlapping response
        void testYesKitNoLegatoNoMono() {
            part->setkititemstatus(1, true);
            part->setkititemstatus(2, true);
            part->kit[1].Padenabled = true;
            part->kit[2].Padenabled = true;
            part->kit[2].Pmaxkey    = 32;
            part->Pkitmode = 1;
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);

            part->notePool.dump();

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[0],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=64,
                    .sendto=0,
                    .size=2,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[1],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=65,
                    .sendto=0,
                    .size=2,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[2],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=0,
                    .sendto=0,
                    .size=0,
                    .status=0,
                    .legatoMirror=false}));

            TS_NON_NULL(part->notePool.sdesc[0].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].kit,  0);

            TS_NON_NULL(part->notePool.sdesc[1].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].kit,  1);

            TS_NON_NULL(part->notePool.sdesc[2].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[2].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[2].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[2].kit,  0);

            TS_NON_NULL(part->notePool.sdesc[3].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[3].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[3].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[3].kit,  1);

            assert_ptr_eq(part->notePool.sdesc[4].note,
                          nullptr,
                          "note free", __LINE__);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[4].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[4].kit,  0);
        }

        void testYesKitYesLegatoNoMono() {
            part->setkititemstatus(1, true);
            part->setkititemstatus(2, true);
            part->kit[1].Padenabled = true;
            part->kit[2].Padenabled = true;
            part->kit[2].Pmaxkey    = 32;
            part->Pkitmode    = 1;
            part->Ppolymode   = false;
            part->Plegatomode = true;
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);

            part->notePool.dump();

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[0],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=65,
                    .sendto=0,
                    .size=2,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[1],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=65,
                    .sendto=0,
                    .size=2,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[2],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=0,
                    .sendto=0,
                    .size=0,
                    .status=0,
                    .legatoMirror=false}));

            TS_NON_NULL(part->notePool.sdesc[0].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].kit,  0);

            TS_NON_NULL(part->notePool.sdesc[1].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].kit,  1);

            TS_NON_NULL(part->notePool.sdesc[2].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[2].note->legato.silent, true);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[2].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[2].kit,  0);

            TS_NON_NULL(part->notePool.sdesc[3].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[3].note->legato.silent, true);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[3].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[3].kit,  1);

            assert_ptr_eq(part->notePool.sdesc[4].note,
                          nullptr,
                          "note free", __LINE__);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[4].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[4].kit,  0);
        }

        void testYesKitNoLegatoYesMono() {
            part->setkititemstatus(1, true);
            part->setkititemstatus(2, true);
            part->kit[1].Padenabled = true;
            part->kit[2].Padenabled = true;
            part->kit[2].Pmaxkey    = 32;
            part->Pkitmode    = 1;
            part->Ppolymode   = false;
            part->Plegatomode = false;
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);

            part->notePool.dump();

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[0],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=64,
                    .sendto=0,
                    .size=2,
                    .status=KEY_RELEASED,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[1],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=65,
                    .sendto=0,
                    .size=2,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[2],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=0,
                    .sendto=0,
                    .size=0,
                    .status=0,
                    .legatoMirror=false}));

            TS_NON_NULL(part->notePool.sdesc[0].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].kit,  0);

            TS_NON_NULL(part->notePool.sdesc[1].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].kit,  1);

            TS_NON_NULL(part->notePool.sdesc[2].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[2].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[2].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[2].kit,  0);

            TS_NON_NULL(part->notePool.sdesc[3].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[3].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[3].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[3].kit,  1);

            assert_ptr_eq(part->notePool.sdesc[4].note,
                          nullptr,
                          "note free", __LINE__);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[4].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[4].kit,  0);
        }

        //Single Kit
        void testSingleKitNoLegatoNoMono() {
            part->setkititemstatus(1, true);
            part->setkititemstatus(2, true);
            part->kit[1].Padenabled = true;
            part->kit[2].Padenabled = true;
            part->kit[2].Pmaxkey    = 32;
            part->Pkitmode = 2;
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);

            part->notePool.dump();

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[0],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=64,
                    .sendto=0,
                    .size=1,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[1],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=65,
                    .sendto=0,
                    .size=1,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[2],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=0,
                    .sendto=0,
                    .size=0,
                    .status=0,
                    .legatoMirror=false}));

            TS_NON_NULL(part->notePool.sdesc[0].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].kit,  0);

            TS_NON_NULL(part->notePool.sdesc[1].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].kit,  0);

            assert_ptr_eq(part->notePool.sdesc[2].note,
                          nullptr,
                          "note free", __LINE__);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[2].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[2].kit,  0);
        }

        void testSingleKitYesLegatoNoMono() {
            part->setkititemstatus(1, true);
            part->setkititemstatus(2, true);
            part->kit[1].Padenabled = true;
            part->kit[2].Padenabled = true;
            part->kit[2].Pmaxkey    = 32;
            part->Pkitmode    = 2;
            part->Ppolymode   = false;
            part->Plegatomode = true;
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[0],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=65,
                    .sendto=0,
                    .size=1,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[1],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=65,
                    .sendto=0,
                    .size=1,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[2],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=0,
                    .sendto=0,
                    .size=0,
                    .status=0,
                    .legatoMirror=false}));

            TS_NON_NULL(part->notePool.sdesc[0].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].kit,  0);

            TS_NON_NULL(part->notePool.sdesc[1].note);
            if(part->notePool.sdesc[1].note)
                TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].note->legato.silent, true);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].kit,  0);
        }

        void testSingleKitNoLegatoYesMono() {
            part->setkititemstatus(1, true);
            part->setkititemstatus(2, true);
            part->kit[1].Padenabled = true;
            part->kit[2].Padenabled = true;
            part->kit[2].Pmaxkey    = 32;
            part->Pkitmode    = 2;
            part->Ppolymode   = false;
            part->Plegatomode = false;
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);



            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[0],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=64,
                    .sendto=0,
                    .size=1,
                    .status=KEY_RELEASED,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[1],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=65,
                    .sendto=0,
                    .size=1,
                    .status=KEY_PLAYING,
                    .legatoMirror=false}));

            TS_ASSERT_EQUAL_CPP(part->notePool.ndesc[2],
                    (NotePool::NoteDescriptor{
                    .age=0,
                    .note=0,
                    .sendto=0,
                    .size=0,
                    .status=0,
                    .legatoMirror=false}));

            TS_NON_NULL(part->notePool.sdesc[0].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[0].kit,  0);

            TS_NON_NULL(part->notePool.sdesc[1].note);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].note->legato.silent, false);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].type, 0);
            TS_ASSERT_EQUAL_INT(part->notePool.sdesc[1].kit,  0);
        }

        void testKeyLimit(void)
        {
            auto &pool = part->notePool;
            //Verify that without a key limit, several notes can be run
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);
            part->NoteOn(66, 127, 0);
            part->NoteOn(67, 127, 0);
            part->NoteOn(68, 127, 0);

            //Verify that notes are spawned as expected
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  5);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 5);

            //Reset the part
            part->monomemClear();
            pool.killAllNotes();

            //Verify that notes are despawned
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  0);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 0);

            //Enable keylimit
            part->setkeylimit(3);

            //Replay notes
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);
            part->NoteOn(66, 127, 0);
            part->NoteOn(67, 127, 0);
            part->NoteOn(68, 127, 0);

            printf("Keylimit 3: After noteons:\n");
            pool.dump();
            //Verify that notes are spawned as expected with limit
            TS_ASSERT_EQUAL_INT(pool.getRunningNotes(),  3);//2 entombed
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),     5);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(),    5);

            //Reset the part
            part->monomemClear();
            pool.killAllNotes();

            //Verify that notes are despawned
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  0);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 0);

            //Now to test note stealing

            //Replay notes
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);
            part->NoteOn(66, 127, 0);

            //Verify that note pool is full
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  3);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 3);

            //Age the notes
            pool.ndesc[1].age = 50;
            pool.ndesc[2].age = 500;

            printf("-------------------------------------\n");

            //Inject two more notes which should steal the note
            //descriptors for #66 and #65
            part->NoteOn(67, 127, 0);
            pool.cleanup();
            TS_ASSERT_EQUAL_INT(pool.ndesc[0].note, 64);
            TS_ASSERT_EQUAL_INT(pool.ndesc[1].note, 65);
            TS_ASSERT_EQUAL_INT(pool.ndesc[2].note, 66);
            TS_ASSERT_EQUAL_INT(pool.ndesc[2].status, KEY_ENTOMBED);
            TS_ASSERT_EQUAL_INT(pool.ndesc[3].note, 67);

            part->NoteOn(68, 127, 0);

            //Verify that note pool is still full and entombed
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  5);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 5);

            //Check that the result is {64, 68, 67}
            TS_ASSERT_EQUAL_INT(pool.ndesc[0].note, 64);
            TS_ASSERT_EQUAL_INT(pool.ndesc[1].note, 65);
            TS_ASSERT_EQUAL_INT(pool.ndesc[1].status, KEY_ENTOMBED);
            TS_ASSERT_EQUAL_INT(pool.ndesc[2].note, 66);
            TS_ASSERT_EQUAL_INT(pool.ndesc[2].status, KEY_ENTOMBED);
            TS_ASSERT_EQUAL_INT(pool.ndesc[3].note, 67);
            TS_ASSERT_EQUAL_INT(pool.ndesc[4].note, 68);
        }

        void testVoiceLimit(void)
        {
            auto &pool = part->notePool;
            //Verify that without a voice limit, several notes can be run
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);
            part->NoteOn(66, 127, 0);
            part->NoteOn(67, 127, 0);
            part->NoteOn(68, 127, 0);

            //Verify that notes are spawned as expected
            TS_ASSERT_EQUAL_INT(pool.getRunningNotes(),  5);
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  5);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 5);

            //Enable voice limit
            part->setvoicelimit(3);

            //Verify that the voice limit has been immediately enforced
            TS_ASSERT_EQUAL_INT(pool.getRunningNotes(),  3);//2 entombed
            pool.dump();

            //With equal age, the notes with the lowest desciptor numbers
            //can be assumed to be "oldest", so verify that they are the
            //ones that are entombed.
            pool.cleanup();
            TS_ASSERT_EQUAL_INT(pool.ndesc[0].note, 64);
            TS_ASSERT_EQUAL_INT(pool.ndesc[0].status, KEY_ENTOMBED);
            TS_ASSERT_EQUAL_INT(pool.ndesc[1].note, 65);
            TS_ASSERT_EQUAL_INT(pool.ndesc[1].status, KEY_ENTOMBED);
            TS_ASSERT_EQUAL_INT(pool.ndesc[2].note, 66);
            TS_ASSERT_EQUAL_INT(pool.ndesc[3].note, 67);
            TS_ASSERT_EQUAL_INT(pool.ndesc[4].note, 68);

            //Reset the part
            part->monomemClear();
            pool.killAllNotes();

            //Verify that notes are despawned
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  0);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 0);

            //Now to test note stealing in real time

            //Play notes
            //Now, the voice limit should be enforced in real time
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);
            part->NoteOn(66, 127, 0);
            part->NoteOn(67, 127, 0);

            //Verify that notes are spawned as expected with limit
            TS_ASSERT_EQUAL_INT(pool.getRunningNotes(),  3);//1 entombed
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),     4);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(),    4);

            //Verify correct one entombed
            pool.cleanup();
            TS_ASSERT_EQUAL_INT(pool.ndesc[0].note, 64);
            TS_ASSERT_EQUAL_INT(pool.ndesc[0].status, KEY_ENTOMBED);
            TS_ASSERT_EQUAL_INT(pool.ndesc[1].note, 65);
            TS_ASSERT_EQUAL_INT(pool.ndesc[2].note, 66);
            TS_ASSERT_EQUAL_INT(pool.ndesc[3].note, 67);

            //One more note
            part->NoteOn(68, 127, 0);

            //Verify that notes are spawned as expected with limit
            TS_ASSERT_EQUAL_INT(pool.getRunningNotes(),  3);//2 entombed
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),     5);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(),    5);

            //Verify correct ones entombed
            pool.cleanup();
            TS_ASSERT_EQUAL_INT(pool.ndesc[0].note, 64);
            TS_ASSERT_EQUAL_INT(pool.ndesc[0].status, KEY_ENTOMBED);
            TS_ASSERT_EQUAL_INT(pool.ndesc[1].note, 65);
            TS_ASSERT_EQUAL_INT(pool.ndesc[1].status, KEY_ENTOMBED);
            TS_ASSERT_EQUAL_INT(pool.ndesc[2].note, 66);
            TS_ASSERT_EQUAL_INT(pool.ndesc[3].note, 67);
            TS_ASSERT_EQUAL_INT(pool.ndesc[4].note, 68);

            //Reset the part
            part->monomemClear();
            pool.killAllNotes();

            //Verify that notes are despawned
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  0);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 0);

            //Now to test that the oldest ones are stolen first

            //Replay notes
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);
            part->NoteOn(66, 127, 0);

            //Verify that note pool is full
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  3);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 3);

            //Age the notes
            pool.ndesc[1].age = 50;
            pool.ndesc[2].age = 500;

            //Inject two more notes which should steal the note
            //descriptors for #66 and #65
            //First one, it should steal the oldest one
            part->NoteOn(67, 127, 0);

            pool.cleanup();
            TS_ASSERT_EQUAL_INT(pool.ndesc[0].note, 64);
            TS_ASSERT_EQUAL_INT(pool.ndesc[1].note, 65);
            TS_ASSERT_EQUAL_INT(pool.ndesc[2].note, 66);
            TS_ASSERT_EQUAL_INT(pool.ndesc[2].status, KEY_ENTOMBED);
            TS_ASSERT_EQUAL_INT(pool.ndesc[3].note, 67);

            //And the the other, it should steal the next-to-oldest one
            part->NoteOn(68, 127, 0);

            //Verify that note pool is full and entombed
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  5);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 5);
            TS_ASSERT_EQUAL_INT(pool.getRunningNotes(),  3);//2 entombed

            //Check that the result is {64, 68, 67}
            pool.cleanup();
            TS_ASSERT_EQUAL_INT(pool.ndesc[0].note, 64);
            TS_ASSERT_EQUAL_INT(pool.ndesc[1].note, 65);
            TS_ASSERT_EQUAL_INT(pool.ndesc[1].status, KEY_ENTOMBED);
            TS_ASSERT_EQUAL_INT(pool.ndesc[2].note, 66);
            TS_ASSERT_EQUAL_INT(pool.ndesc[2].status, KEY_ENTOMBED);
            TS_ASSERT_EQUAL_INT(pool.ndesc[3].note, 67);
            TS_ASSERT_EQUAL_INT(pool.ndesc[4].note, 68);

            //Reset the part
            part->monomemClear();
            pool.killAllNotes();

            //Verify that notes are despawned
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  0);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 0);

            //Now to test that released notes are stolen first, then
            //sustained, regardless of age

            //Replay notes
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);
            part->NoteOn(66, 127, 0);

            //Age the notes
            pool.ndesc[0].age = 50;
            pool.ndesc[1].age = 10;
            pool.ndesc[2].age = 5;
            // Adjust status so we can test priorities
            pool.ndesc[1].status = KEY_RELEASED_AND_SUSTAINED;
            pool.ndesc[2].status = KEY_RELEASED;

            //Verify that note pool is full
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  3);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 3);

            //Inject one note which should steal the note
            //descriptor for #66 (RELEASED) rather than #64 (oldest)
            //or #65 (RELEASED_AND_SUSTAINED)
            part->NoteOn(67, 127, 0);

            //Verify the note pool
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  4);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 4);
            TS_ASSERT_EQUAL_INT(pool.getRunningNotes(),  3);//1 entombed

            pool.cleanup();
            TS_ASSERT_EQUAL_INT(pool.ndesc[0].note, 64);
            TS_ASSERT_EQUAL_INT(pool.ndesc[1].note, 65);
            TS_ASSERT_EQUAL_INT(pool.ndesc[2].note, 66);
            TS_ASSERT_EQUAL_INT(pool.ndesc[2].status, KEY_ENTOMBED);
            TS_ASSERT_EQUAL_INT(pool.ndesc[3].note, 67);

            //Inject yet one more note which should steal the note
            //descriptor for #65 (RELEASED_AND_SUSTAINED) rather than
            //#64 (oldest)
            part->NoteOn(68, 127, 0);

            //Verify the note pool
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  5);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 5);
            TS_ASSERT_EQUAL_INT(pool.getRunningNotes(),  3);//2 entombed

            pool.cleanup();
            pool.dump();
            TS_ASSERT_EQUAL_INT(pool.ndesc[0].note, 64);
            TS_ASSERT_EQUAL_INT(pool.ndesc[1].note, 65);
            TS_ASSERT_EQUAL_INT(pool.ndesc[1].status, KEY_ENTOMBED);
            TS_ASSERT_EQUAL_INT(pool.ndesc[2].note, 66);
            TS_ASSERT_EQUAL_INT(pool.ndesc[2].status, KEY_ENTOMBED);
            TS_ASSERT_EQUAL_INT(pool.ndesc[3].note, 67);
            TS_ASSERT_EQUAL_INT(pool.ndesc[4].note, 68);

            //Reset the part
            part->monomemClear();
            pool.killAllNotes();

            //Verify that notes are despawned
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  0);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 0);

            //Now to test that notes with the same number are
            //stolen before others

            //Replay notes
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);
            part->NoteOn(66, 127, 0);

            //Verify that note pool is full
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  3);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 3);

            //Age the notes and change state
            //Since the note assigner when presented with a note which is
            //already active puts the previous one in the RELEASED state
            //(with the SUSTAIN bit set), prior to voice limiting, we
            //need to put all the notes in the RELEASED state so that
            //we actually test that the voice limiting prioritization
            //between notes in the same state.
            pool.ndesc[0].age = 50;
            pool.ndesc[2].age = 500;
            pool.ndesc[0].status = KEY_RELEASED;
            pool.ndesc[1].status = KEY_RELEASED;
            pool.ndesc[2].status = KEY_RELEASED;

            //Inject another #65 which should steal the note
            //descriptor for the existing #65, even though
            //#66 is older, and #64 is earlier in the list
            part->NoteOn(65, 127, 0);

            //Verify note pool
            TS_ASSERT_EQUAL_INT(pool.usedNoteDesc(),  4);
            TS_ASSERT_EQUAL_INT(pool.usedSynthDesc(), 4);
            TS_ASSERT_EQUAL_INT(pool.getRunningNotes(),  1);//only one playing

            pool.cleanup();
            //The note that we're expecting to be entombed, has previously
            //had its SUSTAIN bit set (see above, and testSustainCase1), so
            //we're expecting that to be retained when the note is entombed.
            TS_ASSERT_EQUAL_INT(pool.ndesc[0].note, 64);
            TS_ASSERT_EQUAL_INT(pool.ndesc[1].note, 65);
            TS_ASSERT_EQUAL_INT(pool.ndesc[1].status, KEY_ENTOMBED|SUSTAIN_BIT);
            TS_ASSERT_EQUAL_INT(pool.ndesc[2].note, 66);
            TS_ASSERT_EQUAL_INT(pool.ndesc[3].note, 65);
        }

        void tearDown() {
            delete part;
            delete[] outL;
            delete[] outR;
            delete time;
            delete synth;
        }
};

int main()
{
    KitTest test;
    RUN_TEST(testSustainCase1);
    RUN_TEST(testSustainCase2);
    RUN_TEST(testMonoSustain);
    RUN_TEST(testNoKitNoLegatoNoMono);
    RUN_TEST(testNoKitYesLegatoNoMono);
    RUN_TEST(testNoKitNoLegatoYesMono);
    RUN_TEST(testYesKitNoLegatoNoMono);
    RUN_TEST(testYesKitYesLegatoNoMono);
    RUN_TEST(testYesKitNoLegatoYesMono);
    RUN_TEST(testSingleKitNoLegatoNoMono);
    RUN_TEST(testSingleKitYesLegatoNoMono);
    RUN_TEST(testSingleKitNoLegatoYesMono);
    RUN_TEST(testKeyLimit);
    RUN_TEST(testVoiceLimit);
    return test_summary();
}
