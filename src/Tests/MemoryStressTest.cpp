/*
  ZynAddSubFX - a software synthesizer

  AdNoteTest.h - CxxTest for Synth/ADnote
  Copyright (C) 2009-2011 Mark McCurry
  Copyright (C) 2009 Harald Hvaal
  Authors: Mark McCurry, Harald Hvaal

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/


#include "test-suite.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include "../Misc/Master.h"
#include "../Misc/Util.h"
#include "../Misc/Allocator.h"
#include "../Synth/ADnote.h"
#include "../Params/Presets.h"
#include "../DSP/FFTwrapper.h"
#include "../globals.h"
using namespace zyn;

SYNTH_T *synth;

class MemoryStressTest
{
    public:
        AbsTime      *time;
        FFTwrapper   *fft;
        ADnoteParameters *defaultPreset;
        Controller   *controller;
        Alloc         memory;

        void setUp() {
            //First the sensible settings and variables that have to be set:
            synth = new SYNTH_T;
            synth->buffersize = 256;
            //synth->alias();
            time  = new AbsTime(*synth);

            fft = new FFTwrapper(synth->oscilsize);
            //prepare the default settings
            defaultPreset = new ADnoteParameters(*synth, fft, time);

            //Assert defaults
            TS_ASSERT(!defaultPreset->VoicePar[1].Enabled);

            std::string instrument_filename = std::string(SOURCE_DIR) + "/guitar-adnote.xmz";
            std::cout << instrument_filename << std::endl;

            XMLwrapper wrap;
            wrap.loadXMLfile(instrument_filename);
            TS_ASSERT(wrap.enterbranch("MASTER"));
            TS_ASSERT(wrap.enterbranch("PART", 0));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT"));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT_KIT"));
            TS_ASSERT(wrap.enterbranch("INSTRUMENT_KIT_ITEM", 0));
            TS_ASSERT(wrap.enterbranch("ADD_SYNTH_PARAMETERS"));
            defaultPreset->getfromXML(wrap);

            //verify xml was loaded
            TS_ASSERT(defaultPreset->VoicePar[1].Enabled);

            controller = new Controller(*synth, time);

        }

        void tearDown() {
            delete controller;
            delete defaultPreset;
            delete fft;
            FFT_cleanup();
            delete synth;
        }

        void testManySimultaneousNotes() {

            unsigned char testnote = 42;
            SynthParams pars{memory, *controller, *synth, *time, 120, 0, testnote / 12.0f, false, prng()};

            std::vector<ADnote*> notes;

            for ( size_t note_idx = 0; note_idx < 1000; ++ note_idx ) {
                try {
                    notes.push_back(new ADnote(defaultPreset, pars));
                } catch (std::exception & e) {
#if defined(DEBUG)
                    std::cerr << "couldn't push note #" << note_idx << std::endl;
#endif
                }
            }

            // If we made it that far, we managed to create many ADnotewithout sigsev

            for (auto note_ptr: notes) {
                delete note_ptr;
            }


        }

};

int main()
{
    MemoryStressTest test;
    RUN_TEST(testManySimultaneousNotes);
    return test_summary();
}
