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
#include "../Misc/PresetExtractor.h"
#include "../Misc/PresetExtractor.cpp"
#include "../Misc/Util.h"
#include "../globals.h"
#include "../UI/NSM.H"
NSM_Client *nsm = 0;
MiddleWare *middleware = 0;

using namespace std;

char *instance_name=(char*)"";

#define NUM_MIDDLEWARE 3

class MessageTest:public CxxTest::TestSuite
{
    public:
        Config config;
        void setUp() {
            synth = new SYNTH_T;
            mw    = new MiddleWare(std::move(*synth), &config);
            ms    = mw->spawnMaster();
        }

        void tearDown() {
            delete mw;
            delete synth;
        }

        void notestKitEnable(void)
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

        void notestBankCapture(void)
        {
            mw->transmitMsg("/bank/slots", "");
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

            //Paste to PADsynth
            printf("====Paste to PADsynth\n");
            mw->transmitMsg("/presets/paste", "s", "/part0/kit0/padpars/oscilgen/");
            do_exit = 1;
            t.join();

        }

    private:
        SYNTH_T *synth;
        MiddleWare *mw;
        Master     *ms;
};
