/*
  ZynAddSubFX - a software synthesizer

  PluginTest.h - CxxTest for embedding zyn
  Copyright (C) 2013-2013 Mark McCurry
  Authors: Mark McCurry

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
#include <cxxtest/TestSuite.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <rtosc/thread-link.h>
#include <unistd.h>
#include "../Misc/MiddleWare.h"
#include "../Misc/Master.h"
#include "../Misc/Part.h"
#include "../Misc/PresetExtractor.h"
#include "../Misc/PresetExtractor.cpp"
#include "../Misc/Util.h"
#include "../globals.h"
#include "../UI/NSM.H"
class NSM_Client *nsm = 0;
MiddleWare *middleware = 0;

using namespace std;

char *instance_name=(char*)"";

#define NUM_MIDDLEWARE 3

class MessageTest:public CxxTest::TestSuite
{
    public:
        Config config;
        void setUp() {
            synth    = new SYNTH_T;
            mw       = new MiddleWare(std::move(*synth), &config);
            ms       = mw->spawnMaster();
            realtime = NULL;
        }

        void tearDown() {
            delete mw;
            delete synth;
        }

#if 0
        void _testKitEnable(void)
        {
            const char *msg = NULL;
            mw->transmitMsg("/part0/kit0/Psubenabled", "T");
            TS_ASSERT(ms->uToB->hasNext());
            msg = ms->uToB->read();
            TS_ASSERT_EQUALS(string("/part0/kit0/subpars-data"), msg);
            TS_ASSERT(ms->uToB->hasNext());
            msg = ms->uToB->read();
            TS_ASSERT_EQUALS(string("/part0/kit0/Psubenabled"), msg);
        }

        void _testBankCapture(void)
        {
            mw->transmitMsg("/bank/slots", "");
            TS_ASSERT(!ms->uToB->hasNext());
            mw->transmitMsg("/bank/fake", "");
            TS_ASSERT(ms->uToB->hasNext());
            const char *msg = ms->uToB->read();
            TS_ASSERT_EQUALS(string("/bank/fake"), msg);
        }

        void _testOscCopyPaste(void)
        {
            //Enable pad synth
            mw->transmitMsg("/part0/kit0/Ppadenabled", "T");

            TS_ASSERT(ms->uToB->hasNext());
            ms->applyOscEvent(ms->uToB->read());
            TS_ASSERT(ms->uToB->hasNext());
            ms->applyOscEvent(ms->uToB->read());
            TS_ASSERT(!ms->uToB->hasNext());

            ms->part[0]->kit[0].adpars->VoicePar[0].FMSmp->Pbasefuncpar = 32;

            int do_exit = 0;
            std::thread t([&do_exit,this](){
                    int tries = 0;
                    while(tries < 10000 && do_exit == 0) {
                        if(!ms->uToB->hasNext()) {
                            usleep(500);
                            continue;
                        }
                        const char *msg = ms->uToB->read();
                        printf("RT: handling <%s>\n", msg);
                        ms->applyOscEvent(msg);
                    }});

            //Copy From ADsynth modulator
            printf("====Copy From ADsynth modulator\n");
            mw->transmitMsg("/presets/copy", "s", "/part0/kit0/adpars/VoicePar0/FMSmp/");

            TS_ASSERT(ms->part[0]->kit[0].padpars->oscilgen->Pbasefuncpar != 32);
            //Paste to PADsynth
            printf("====Paste to PADsynth\n");
            mw->transmitMsg("/presets/paste", "s", "/part0/kit0/padpars/oscilgen/");
            do_exit = 1;
            t.join();
            TS_ASSERT_EQUALS(ms->part[0]->kit[0].padpars->oscilgen->Pbasefuncpar, 32);
        }
#endif

        void start_realtime(void)
        {
            do_exit = false;
            realtime = new std::thread([this](){
                    int tries = 0;
                    while(tries < 10000) {
                        if(!ms->uToB->hasNext()) {
                            if(do_exit)
                                break;

                            usleep(500);
                            continue;
                        }
                        const char *msg = ms->uToB->read();
                        printf("RT: handling <%s>\n", msg);
                        ms->applyOscEvent(msg);
                    }});
        }

        void stop_realtime(void)
        {
            do_exit = true;
            realtime->join();
            delete realtime;
            realtime = NULL;
        }

        void run_realtime(void)
        {
            start_realtime();
            stop_realtime();
        }

        void testMidiLearn(void)
        {
            mw->transmitMsg("/learn", "s", "/Pvolume");
            mw->transmitMsg("/virtual_midi_cc", "iii", 0, 23, 108);
            TS_ASSERT_EQUALS(ms->Pvolume, 80);

            //Perform a learning operation

            run_realtime(); //1. runs learning and identifies a CC to bind
            mw->tick();     //2. produces new binding table
            run_realtime(); //3. applies new binding table


            //Verify that the learning actually worked
            mw->transmitMsg("/virtual_midi_cc", "iii", 0, 23, 13);
            run_realtime();
            TS_ASSERT_EQUALS(ms->Pvolume, 13);

            mw->transmitMsg("/virtual_midi_cc", "iii", 0, 23, 2);
            run_realtime();
            TS_ASSERT_EQUALS(ms->Pvolume, 2);

            mw->transmitMsg("/virtual_midi_cc", "iii", 0, 23, 0);
            run_realtime();
            TS_ASSERT_EQUALS(ms->Pvolume, 0);

            mw->transmitMsg("/virtual_midi_cc", "iii", 0, 23, 127);
            run_realtime();
            TS_ASSERT_EQUALS(ms->Pvolume, 127);
        }

        void testMidiLearnSave(void)
        {
            mw->transmitMsg("/learn", "s", "/Pvolume");
            mw->transmitMsg("/virtual_midi_cc", "iii", 0, 23, 108);

            //Perform a learning operation

            run_realtime(); //1. runs learning and identifies a CC to bind
            mw->tick();     //2. produces new binding table
            run_realtime(); //3. applies new binding table

            mw->transmitMsg("/save_xlz", "s", "test-midi-learn.xlz");
            mw->transmitMsg("/load_xlz", "s", "test-midi-learn.xlz");
        }

    private:
        SYNTH_T     *synth;
        MiddleWare  *mw;
        Master      *ms;
        std::thread *realtime;
        bool         do_exit;
};
