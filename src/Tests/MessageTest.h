/*
  ZynAddSubFX - a software synthesizer

  PluginTest.h - CxxTest for embedding zyn
  Copyright (C) 2013-2013 Mark McCurry
  Authors: Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
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
using namespace std;
using namespace zyn;

class NSM_Client *nsm = 0;
MiddleWare *middleware = 0;

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

        void testKitEnable(void)
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

        void testBankCapture(void)
        {
            mw->transmitMsg("/bank/slot23", "");
            TS_ASSERT(!ms->uToB->hasNext());
            mw->transmitMsg("/bank/fake", "");
            TS_ASSERT(ms->uToB->hasNext());
            const char *msg = ms->uToB->read();
            TS_ASSERT_EQUALS(string("/bank/fake"), msg);
        }

        void testOscCopyPaste(void)
        {
            //Enable pad synth
            mw->transmitMsg("/part0/kit0/Ppadenabled", "T");

            TS_ASSERT(ms->uToB->hasNext());
            ms->applyOscEvent(ms->uToB->read());
            TS_ASSERT(ms->uToB->hasNext());
            ms->applyOscEvent(ms->uToB->read());
            TS_ASSERT(!ms->uToB->hasNext());

            auto &osc_src = *ms->part[0]->kit[0].adpars->VoicePar[0].FmGn;
            auto &osc_dst = *ms->part[0]->kit[0].padpars->oscilgen;
            auto &osc_oth = *ms->part[0]->kit[0].adpars->VoicePar[1].OscilGn;

            TS_ASSERT_EQUALS(osc_src.Pbasefuncpar, 64);
            osc_src.Pbasefuncpar = 32;
            TS_ASSERT_EQUALS(osc_src.Pbasefuncpar, 32);

            //Copy From ADsynth modulator
            printf("====Copy From ADsynth modulator\n");
            start_realtime();
            mw->transmitMsg("/presets/copy", "s", "/part0/kit0/adpars/VoicePar0/FMSmp/");

            TS_ASSERT_EQUALS(osc_dst.Pbasefuncpar, 64);
            TS_ASSERT_EQUALS(osc_oth.Pbasefuncpar, 64);

            //Paste to PADsynth
            printf("====Paste to PADsynth\n");
            mw->transmitMsg("/presets/paste", "s", "/part0/kit0/padpars/oscilgen/");

            printf("====Paste to ADsynth\n");
            mw->transmitMsg("/presets/paste", "s", "/part0/kit0/adpars/VoicePar1/OscilSmp/");

            stop_realtime();
            TS_ASSERT_EQUALS(osc_dst.Pbasefuncpar, 32);
            TS_ASSERT_EQUALS(osc_oth.Pbasefuncpar, 32);
        }


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
            mw->transmitMsg("/learn", "s", "/Pkeyshift");
            mw->transmitMsg("/virtual_midi_cc", "iii", 0, 23, 108);
            TS_ASSERT_EQUALS(ms->Pkeyshift, 64);

            //Perform a learning operation

            run_realtime(); //1. runs learning and identifies a CC to bind
            mw->tick();     //2. produces new binding table
            run_realtime(); //3. applies new binding table


            //Verify that the learning actually worked
            mw->transmitMsg("/virtual_midi_cc", "iii", 0, 23, 13);
            run_realtime();
            TS_ASSERT_EQUALS(ms->Pkeyshift, 13);

            mw->transmitMsg("/virtual_midi_cc", "iii", 0, 23, 2);
            run_realtime();
            TS_ASSERT_EQUALS(ms->Pkeyshift, 2);

            mw->transmitMsg("/virtual_midi_cc", "iii", 0, 23, 0);
            run_realtime();
            TS_ASSERT_EQUALS(ms->Pkeyshift, 0);

            mw->transmitMsg("/virtual_midi_cc", "iii", 0, 23, 127);
            run_realtime();
            TS_ASSERT_EQUALS(ms->Pkeyshift, 127);
        }

        void testMidiLearnSave(void)
        {
            mw->transmitMsg("/learn", "s", "/Pkeyshift");
            mw->transmitMsg("/virtual_midi_cc", "iii", 0, 23, 108);

            //param is at default until rt-thread is run
            TS_ASSERT_EQUALS(ms->Pkeyshift, 64);


            //Perform a learning operation
            run_realtime();

            //Verify binding affects control
            TS_ASSERT_EQUALS(ms->Pkeyshift, 108);


            printf("# Trying to save automations\n");
            start_realtime();
            mw->transmitMsg("/save_xlz", "s", "test-midi-learn.xlz");
            stop_realtime();

            //Verify that some file exists
            printf("# Verifying file exists\n");
            FILE *f = fopen("test-midi-learn.xlz", "r");
            TS_ASSERT(f);

            if(f)
                fclose(f);

            printf("# Clearing automation\n");
            //Clear out state
            mw->transmitMsg("/clear_xlz", "");
            //Send dummy message
            mw->transmitMsg("/virtual_midi_cc", "iii", 0, 23, 27);
            run_realtime();

            //Verify automation table is clear
            TS_ASSERT_EQUALS(ms->Pkeyshift, 108);

            printf("# Loading automation\n");
            mw->transmitMsg("/load_xlz", "s", "test-midi-learn.xlz");
            //Send message
            mw->transmitMsg("/virtual_midi_cc", "iii", 0, 23, 28);
            run_realtime();

            //Verify automation table is restored
            TS_ASSERT_EQUALS(ms->Pkeyshift, 28);
        }

        void testLfoPaste(void)
        {
            start_realtime();
            ms->part[0]->kit[0].adpars->GlobalPar.FreqLfo->Pfreqrand = 32;
            TS_ASSERT_EQUALS(ms->part[0]->kit[0].adpars->GlobalPar.FreqLfo->Pfreqrand, 32);

            //Copy
            mw->transmitMsg("/presets/copy", "s", "/part0/kit0/adpars/GlobalPar/FreqLfo/");

            ms->part[0]->kit[0].adpars->GlobalPar.FreqLfo->Pfreqrand = 99;
            TS_ASSERT_EQUALS(ms->part[0]->kit[0].adpars->GlobalPar.FreqLfo->Pfreqrand, 99);

            //Paste
            mw->transmitMsg("/presets/paste", "s", "/part0/kit0/adpars/GlobalPar/FreqLfo/");
            stop_realtime();

            TS_ASSERT_EQUALS(ms->part[0]->kit[0].adpars->GlobalPar.FreqLfo->Pfreqrand, 32);
        }

        void testPadPaste(void)
        {
            mw->transmitMsg("/part0/kit0/Ppadenabled", "T");
            run_realtime();

            start_realtime();

            auto &field1 = ms->part[0]->kit[0].padpars->PVolume;
            auto &field2 = ms->part[0]->kit[0].padpars->oscilgen->Pfilterpar1;
            field1 = 32;
            TS_ASSERT_EQUALS(field1, 32);
            field2 = 35;
            TS_ASSERT_EQUALS(field2, 35);

            //Copy
            mw->transmitMsg("/presets/copy", "s", "/part0/kit0/padpars/");

            field1 = 99;
            TS_ASSERT_EQUALS(field1, 99);
            field2 = 95;
            TS_ASSERT_EQUALS(field2, 95);

            //Paste
            mw->transmitMsg("/presets/paste", "s", "/part0/kit0/padpars/");
            stop_realtime();

            TS_ASSERT_EQUALS(field1, 32);
            TS_ASSERT_EQUALS(field2, 35);
        }

        void testFilterDepricated(void)
        {
            vector<string> v = {"Pfreq", "Pfreqtrack", "Pgain", "Pq"};
            for(size_t i=0; i<v.size(); ++i) {
                string path = "/part0/kit0/adpars/GlobalPar/GlobalFilter/"+v[i];
                for(int j=0; j<128; ++j) {
                    mw->transmitMsg(path.c_str(), "i", j); //Set
                    mw->transmitMsg(path.c_str(), ""); //Get
                }

            }
            while(ms->uToB->hasNext()) {
                const char *msg = ms->uToB->read();
                //printf("RT: handling <%s>\n", msg);
                ms->applyOscEvent(msg);
            }

            int id = 0;
            int state = 0;
            int value = 0;
            // 0 - broadcast
            // 1 - true value     (set)
            // 2 - expected value (get)
            while(ms->bToU->hasNext()) {
                const char *msg = ms->bToU->read();
                if(state == 0) {
                    TS_ASSERT_EQUALS(rtosc_narguments(msg), 0U);
                    state = 1;
                } else if(state == 1) {
                    TS_ASSERT_EQUALS(rtosc_narguments(msg), 1U);
                    value = rtosc_argument(msg, 0).i;
                    state = 2;
                } else if(state == 2) {
                    int val = rtosc_argument(msg, 0).i;
                    if(value != val) {
                        printf("%s - %d should equal %d\n", msg, value, val);
                        TS_ASSERT(0);
                    }
                    state = 0;
                }

                (void) id;
                //printf("Message #%d %s:%s\n", id++, msg, rtosc_argument_string(msg));
                //if(rtosc_narguments(msg))
                //    printf("        %d\n", rtosc_argument(msg, 0).i);
            }
        }


    private:
        SYNTH_T     *synth;
        MiddleWare  *mw;
        Master      *ms;
        std::thread *realtime;
        bool         do_exit;
};
