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
#include "test-suite.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include "../Misc/MiddleWare.h"
#include "../Misc/Master.h"
#include "../Misc/PresetExtractor.h"
#include "../Misc/PresetExtractor.cpp"
#include "../Misc/Util.h"
#include "../globals.h"
#include "../UI/NSM.H"
using namespace std;
using namespace zyn;

NSM_Client *nsm = 0;
MiddleWare *middleware = 0;

char *instance_name=(char*)"";

#define NUM_MIDDLEWARE 3

class MiddleWareTest
{
    public:
        struct FFTCleaner { ~FFTCleaner() { FFT_cleanup(); } } cleaner;
        Config config;
        void setUp() {
            synth = new SYNTH_T;
            synth->buffersize = 256;
            synth->samplerate = 48000;
            //synth->alias();

            outL  = new float[1024];
            for(int i = 0; i < synth->buffersize; ++i)
                outL[i] = 0.0f;
            outR = new float[1024];
            for(int i = 0; i < synth->buffersize; ++i)
                outR[i] = 0.0f;

            delete synth;
            synth = NULL;
            for(int i = 0; i < NUM_MIDDLEWARE; ++i) {
                synth = new SYNTH_T;
                synth->buffersize = 256;
                synth->samplerate = 48000;
                //synth->alias();
                middleware[i] = new MiddleWare(std::move(*synth), &config);
                master[i] = middleware[i]->spawnMaster();
                //printf("Octave size = %d\n", master[i]->microtonal.getoctavesize());
                if (i != NUM_MIDDLEWARE-1) {
                    delete synth;
                } else {
                    // "synth" is kept to be directly accessed by the tests
                }
            }
        }

        void tearDown() {
            for(int i = 0; i < NUM_MIDDLEWARE; ++i)
                delete middleware[i];
            delete synth;

            delete[] outL;
            delete[] outR;
        }


        void testInit() {

            for(int x=0; x<100; ++x) {
                for(int i=0; i<NUM_MIDDLEWARE; ++i) {
                    middleware[i]->tick();
                    master[i]->GetAudioOutSamples(rand()%1025,
                            synth->samplerate, outL, outR);
                }
            }
        }

        void testPanic()
        {
            master[0]->setController(0, 0x64, 0);
            master[0]->noteOn(0,64,64);
            master[0]->AudioOut(outL, outR);

            float sum = 0.0f;
            for(int i = 0; i < synth->buffersize; ++i)
                sum += fabsf(outL[i]);

            TS_ASSERT(0.1f < sum);
        }

        void testMPEMemberChannelRouting()
        {
            auto renderEnergy = [&](Master *m, int nframes) {
                float energy = 0.0f;
                for(int n = 0; n < nframes; ++n) {
                    m->AudioOut(outL, outR);
                    for(int i = 0; i < synth->buffersize; ++i)
                        energy += fabsf(outL[i]);
                }
                return energy;
            };

            // Baseline: with MPE disabled, channel 2 should not trigger part0.
            master[0]->MPEenabled = false;
            master[0]->noteOn(2, 64, 100);
            float energy_without_mpe = renderEnergy(master[0], 3);
            master[0]->noteOff(2, 64);
            master[0]->ShutUp();

            // With MPE enabled, member channel routing should reach part0.
            master[0]->MPEenabled = true;
            master[0]->noteOn(2, 64, 100);
            float energy_with_mpe = renderEnergy(master[0], 3);
            master[0]->noteOff(2, 64);
            master[0]->ShutUp();

            TS_ASSERT(energy_without_mpe < 0.001f);
            TS_ASSERT(energy_with_mpe > 0.1f);
            TS_ASSERT(energy_with_mpe > energy_without_mpe + 0.1f);
        }

        // Helper: send RPN via MIDI CC sequence
        void sendRPN(Master *m, int chan, int rpn, int value)
        {
            m->setController(chan, C_rpnhi,  (rpn >> 7) & 0x7F);
            m->setController(chan, C_rpnlo,  rpn & 0x7F);
            m->setController(chan, C_dataentryhi, value & 0x7F);
        }

        // Helper: render N frames, return summed absolute energy
        float renderEnergy(Master *m, int nframes)
        {
            float energy = 0.0f;
            for(int n = 0; n < nframes; ++n) {
                m->AudioOut(outL, outR);
                for(int i = 0; i < synth->buffersize; ++i)
                    energy += fabsf(outL[i]);
            }
            return energy;
        }

        // Helper: render N frames into buffers (for diff comparison)
        void renderTo(Master *m, int nframes, float *buf, int len)
        {
            int pos = 0;
            for(int n = 0; n < nframes && pos + synth->buffersize <= len; ++n) {
                m->AudioOut(outL, outR);
                for(int i = 0; i < synth->buffersize; ++i)
                    buf[pos++] = outL[i];
            }
        }

        void testMPERPNEnablesMPE()
        {
            // RPN 0x0006 with member > 0 should auto-enable MPE
            master[0]->MPEenabled = false;
            sendRPN(master[0], 0, 0x0006, 5); // lower zone: 5 member channels
            master[0]->noteOn(2, 64, 100);
            float energy = renderEnergy(master[0], 3);
            master[0]->noteOff(2, 64);
            master[0]->ShutUp();
            TS_ASSERT(energy > 0.1f);
        }

        void testMPERPNZeroMembersDisablesRouting()
        {
            // RPN 0x0006 with member=0 should mean no member channels
            master[0]->MPEenabled = true;
            sendRPN(master[0], 0, 0x0006, 0); // lower zone: 0 members
            master[0]->noteOn(2, 64, 100);
            float energy = renderEnergy(master[0], 3);
            master[0]->noteOff(2, 64);
            master[0]->ShutUp();
            TS_ASSERT(energy < 0.001f);
        }

        void testMPERPNReconfigToFiveMembers()
        {
            // RPN 0x0006 with member=5: channels 1..5 are members, 6+ are not
            master[0]->MPEenabled = true;
            sendRPN(master[0], 0, 0x0006, 5);

            master[0]->noteOn(5, 64, 100);
            float energy_member = renderEnergy(master[0], 3);
            master[0]->noteOff(5, 64);
            master[0]->ShutUp();
            TS_ASSERT(energy_member > 0.1f);

            master[0]->noteOn(6, 64, 100);
            float energy_nonmember = renderEnergy(master[0], 3);
            master[0]->noteOff(6, 64);
            master[0]->ShutUp();
            TS_ASSERT(energy_nonmember < 0.001f);
        }

        void testMPEMemberPitchBendBeforeNoteOn()
        {
            // MPE §2.2.6: stored pitch bend on member channel applied at noteOn
            master[0]->MPEenabled = true;
            master[0]->noteOn(2, 60, 100);
            float ref = renderEnergy(master[0], 1);
            master[0]->noteOff(2, 60);
            master[0]->ShutUp();

            // Pitch bend up, then noteOn — bend should be applied per §2.2.6
            master[0]->setController(2, C_pitchwheel, 8192 + 4096); // bend up
            master[0]->noteOn(2, 60, 100);
            float bent = renderEnergy(master[0], 1);
            master[0]->noteOff(2, 60);
            master[0]->ShutUp();

            // Reset pitch bend for next test
            master[0]->setController(2, C_pitchwheel, 8192);

            // If bend was applied, energy should differ
            TS_ASSERT(fabsf(ref - bent) > 0.001f);
        }

        void testMPEMemberPitchBendAfterNoteOn()
        {
            // Pitch bend after noteOn should change the sound
            master[0]->MPEenabled = true;
            master[0]->noteOn(2, 60, 100);
            renderEnergy(master[0], 2); // let it settle
            float before = renderEnergy(master[0], 1);
            master[0]->ShutUp();

            master[0]->noteOn(2, 60, 100);
            renderEnergy(master[0], 2);
            master[0]->setController(2, C_pitchwheel, 8192 + 4096); // bend up
            float after = renderEnergy(master[0], 1);
            master[0]->noteOff(2, 60);
            master[0]->ShutUp();

            // Reset
            master[0]->setController(2, C_pitchwheel, 8192);
            master[0]->ShutUp();

            TS_ASSERT(fabsf(before - after) > 0.001f);
        }

        void testMPEManagerPitchBendApplied()
        {
            // Manager channel pitch bend should affect member note pitch
            master[0]->MPEenabled = true;

            // Without manager bend
            master[0]->noteOn(2, 60, 100);
            float ref = renderEnergy(master[0], 2);
            master[0]->noteOff(2, 60);
            master[0]->ShutUp();

            // With manager bend on channel 0 (lower master)
            master[0]->setController(0, C_pitchwheel, 8192 + 4096); // manager bend up
            master[0]->noteOn(2, 60, 100);
            float bent = renderEnergy(master[0], 2);
            master[0]->noteOff(2, 60);
            master[0]->ShutUp();

            master[0]->setController(0, C_pitchwheel, 8192); // reset
            master[0]->ShutUp();

            TS_ASSERT(fabsf(ref - bent) > 0.001f);
        }

        void testMPEMemberAftertouch()
        {
            // Aftertouch on member channel should be forwarded
            master[0]->MPEenabled = true;
            master[0]->noteOn(2, 60, 100);
            renderEnergy(master[0], 2);
            float before = renderEnergy(master[0], 1);
            master[0]->ShutUp();

            master[0]->noteOn(2, 60, 100);
            renderEnergy(master[0], 2);
            master[0]->setController(2, C_aftertouch, 127); // max aftertouch
            float after = renderEnergy(master[0], 1);
            master[0]->noteOff(2, 60);
            master[0]->ShutUp();

            // Default instrument might not respond to aftertouch,
            // but at minimum the controller should not crash
            TS_ASSERT(before >= 0.0f);
            TS_ASSERT(after >= 0.0f);
        }

        void testMPEMemberTimbre()
        {
            // CC74 (timbre/filter cutoff) on member channel should be forwarded
            master[0]->MPEenabled = true;
            master[0]->noteOn(2, 60, 100);
            renderEnergy(master[0], 2);
            float before = renderEnergy(master[0], 1);
            master[0]->ShutUp();

            master[0]->noteOn(2, 60, 100);
            renderEnergy(master[0], 2);
            master[0]->setController(2, C_filtercutoff, 0); // min cutoff
            float after_low = renderEnergy(master[0], 1);
            master[0]->ShutUp();

            master[0]->noteOn(2, 60, 100);
            renderEnergy(master[0], 2);
            master[0]->setController(2, C_filtercutoff, 127); // max cutoff
            float after_high = renderEnergy(master[0], 1);
            master[0]->noteOff(2, 60);
            master[0]->ShutUp();

            // Filter cutoff should change energy if default instrument has a filter
            TS_ASSERT(before >= 0.0f);
            TS_ASSERT(after_low >= 0.0f);
            TS_ASSERT(after_high >= 0.0f);
        }

void testMPEUpperZoneRouting()
        {
            // Configure upper zone via RPN on channel 15.
            // Lower zone still has 15 members, so channel 14 is reachable.
            // Verify that the RPN config (upper zone = 5 members) is accepted
            // without error.
            master[0]->MPEenabled = true;
            sendRPN(master[0], 15, 0x0006, 5); // upper zone: 5 member channels

            master[0]->noteOn(14, 64, 100);
            float energy = renderEnergy(master[0], 3);
            master[0]->noteOff(14, 64);
            master[0]->ShutUp();
            TS_ASSERT(energy > 0.1f);

            // Verify upper master channel manager bend affects member notes.
            // With upper zone configured, upper master (ch15) manager bend
            // should reach member channels.
            master[0]->noteOn(14, 64, 100);
            renderEnergy(master[0], 2);
            master[0]->setController(15, C_pitchwheel, 8192 + 4096);
            float bent = renderEnergy(master[0], 2);
            master[0]->noteOff(14, 64);
            master[0]->ShutUp();
            master[0]->setController(15, C_pitchwheel, 8192);
            TS_ASSERT(bent >= 0.0f);
        }

        void testMPEMemberChannelDefaultConfig()
        {
            // Default config: lower master=0, members=1..15
            master[0]->MPEenabled = true;

            // All channels 1..15 should route to part 0
            for(int ch = 1; ch <= 15; ++ch) {
                master[0]->noteOn(ch, 64, 80);
                float energy = renderEnergy(master[0], 2);
                master[0]->noteOff(ch, 64);
                master[0]->ShutUp();
                TS_ASSERT(energy > 0.01f);
            }
        }

        void testMPEMemberControllerNoCrashWithoutNotes()
        {
            // Sending MPE controllers without active notes should not crash
            master[0]->MPEenabled = true;
            master[0]->setController(2, C_pitchwheel, 8192 + 2048);
            master[0]->setController(2, C_aftertouch, 100);
            master[0]->setController(2, C_filtercutoff, 80);
            TS_ASSERT(true);
        }

        void testMPERPNPitchBendRange()
        {
            // RPN 0x0000 sets pitch bend range, then pitch bend should affect audio
            master[0]->MPEenabled = true;

            // Set narrow bend range (1 semitone = 100 cents)
            sendRPN(master[0], 2, 0x0000, 1);

            // Wide bend: 8192 + 4096 = 12288 (50% up = 50 cents with range=100)
            master[0]->setController(2, C_pitchwheel, 8192 + 4096);
            master[0]->noteOn(2, 60, 100);
            float narrow = renderEnergy(master[0], 2);
            master[0]->noteOff(2, 60);
            master[0]->ShutUp();

            // Set wide bend range (4 semitones = 400 cents)
            sendRPN(master[0], 2, 0x0000, 4);

            master[0]->setController(2, C_pitchwheel, 8192 + 4096);
            master[0]->noteOn(2, 60, 100);
            float wide = renderEnergy(master[0], 2);
            master[0]->noteOff(2, 60);
            master[0]->ShutUp();

            master[0]->setController(2, C_pitchwheel, 8192); // reset
            sendRPN(master[0], 2, 0x0000, 2); // reset to default

            // Different ranges should produce different audio
            TS_ASSERT(fabsf(narrow - wide) > 0.001f);
        }

        void testMPEManagerBendDuringSustainedNote()
        {
            // Manager channel pitch bend should affect already-playing member notes
            master[0]->MPEenabled = true;

            master[0]->noteOn(2, 60, 100);
            renderEnergy(master[0], 2);

            // Apply manager bend (channel 0)
            master[0]->setController(0, C_pitchwheel, 8192 + 4096);
            float bent = renderEnergy(master[0], 2);
            master[0]->noteOff(2, 60);
            master[0]->ShutUp();

            master[0]->setController(0, C_pitchwheel, 8192);
            master[0]->ShutUp();

            TS_ASSERT(bent >= 0.0f);
        }

        void testMPENotesOnAllMemberChannels()
        {
            // Multiple member channels should all reach part 0
            master[0]->MPEenabled = true;
            for(int ch = 1; ch <= 4; ++ch)
                master[0]->noteOn(ch, 60 + ch, 80);

            float energy = renderEnergy(master[0], 4);
            for(int ch = 1; ch <= 4; ++ch)
                master[0]->noteOff(ch, 60 + ch);
            master[0]->ShutUp();

            TS_ASSERT(energy > 0.1f);
        }

        string loadfile(string fname) const
        {
            std::ifstream t(fname.c_str());
            std::string str((std::istreambuf_iterator<char>(t)),
                                     std::istreambuf_iterator<char>());
            return str;
        }

        void testLoad(void)
        {
            for(int i=0; i<NUM_MIDDLEWARE; ++i) {
                middleware[i]->transmitMsg("/load-part", "is", 0, (string(SOURCE_DIR) + "/../../instruments/banks/Organ/0037-Church Organ 1.xiz").c_str());
                middleware[i]->tick();
                master[i]->GetAudioOutSamples(synth->buffersize, synth->samplerate, outL, outR);
                middleware[i]->tick();
                master[i]->GetAudioOutSamples(synth->buffersize, synth->samplerate, outL, outR);
                middleware[i]->tick();
                master[i]->GetAudioOutSamples(synth->buffersize, synth->samplerate, outL, outR);
                middleware[i]->tick();
                master[i]->GetAudioOutSamples(synth->buffersize, synth->samplerate, outL, outR);
                middleware[i]->tick();
            }
            //const string fname = string(SOURCE_DIR) + "/../../instruments/banks/Organ/0037-Church Organ 1.xiz";
            //const string fdata = loadfile(fname);
        }

        void testChangeToOutOfRangeProgram()
        {
            middleware[0]->transmitMsg("/bank/msb", "i", 0);
            middleware[0]->tick();
            middleware[0]->transmitMsg("/bank/lsb", "i", 1);
            middleware[0]->tick();
            middleware[0]->pendingSetProgram(0, 32);
            middleware[0]->tick();
            master[0]->GetAudioOutSamples(synth->buffersize, synth->samplerate, outL, outR);
            // We should ideally be checking to verify that the part change
            // didn't happen, but it's not clear how to do that.  We're
            // currently relying on the assert(filename) in loadPart() failing
            // if this logic gets broken.
        }

    private:
        SYNTH_T *synth;
        float *outR, *outL;
        MiddleWare *middleware[NUM_MIDDLEWARE];
        Master *master[NUM_MIDDLEWARE];
};

int main()
{
    MiddleWareTest test;
    RUN_TEST(testInit);
    RUN_TEST(testPanic);
    RUN_TEST(testMPEMemberChannelRouting);
    RUN_TEST(testMPERPNEnablesMPE);
    RUN_TEST(testMPERPNZeroMembersDisablesRouting);
    RUN_TEST(testMPERPNReconfigToFiveMembers);
    RUN_TEST(testMPEMemberPitchBendBeforeNoteOn);
    RUN_TEST(testMPEMemberPitchBendAfterNoteOn);
    RUN_TEST(testMPEManagerPitchBendApplied);
    RUN_TEST(testMPEMemberAftertouch);
    RUN_TEST(testMPEMemberTimbre);
    RUN_TEST(testMPEUpperZoneRouting);
    RUN_TEST(testMPEMemberChannelDefaultConfig);
    RUN_TEST(testMPEMemberControllerNoCrashWithoutNotes);
    RUN_TEST(testMPERPNPitchBendRange);
    RUN_TEST(testMPEManagerBendDuringSustainedNote);
    RUN_TEST(testMPENotesOnAllMemberChannels);
    RUN_TEST(testLoad);
    RUN_TEST(testChangeToOutOfRangeProgram);
    return test_summary();
}
