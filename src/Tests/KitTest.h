#include <cxxtest/TestSuite.h>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include "../Misc/Allocator.h"
#include "../DSP/FFTwrapper.h"
#include "../Misc/Microtonal.h"
#define private public
#define protected public
#include "../Synth/SynthNote.h"
#include "../Misc/Part.h"
#include "../globals.h"
SYNTH_T *synth;
extern float *denormalkillbuf;

using namespace std;

class KitTest:public CxxTest::TestSuite
{
    private:
        Allocator alloc;
        FFTwrapper fft;
        Microtonal microtonal;
        Part *part;
        float *outL, *outR;
    public:
        KitTest()
            :fft(512)
        {}
        void setUp() {
            synth = new SYNTH_T;
            denormalkillbuf = new float[synth->buffersize];
            outL  = new float[synth->buffersize];
            outR = new float[synth->buffersize];
            memset(denormalkillbuf, 0, synth->bufferbytes);
            memset(outL, 0, synth->bufferbytes);
            memset(outR, 0, synth->bufferbytes);


            part = new Part(alloc, *synth, &microtonal, &fft);
        }

        //Enumerate cases of:
        //Legato = {disabled,enabled}
        //Mono   = {diabled, enabled}
        //Kit    = {off, normal, single}
        //ignore legato=enabled, mono=enabled

        //No Kit
        void testNoKitNoLegatoNoMono() {
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);

            TS_ASSERT_EQUALS(part->partnote[0].status, Part::KEY_PLAYING);
            TS_ASSERT_EQUALS(part->partnote[0].note, 64);
            TS_ASSERT_EQUALS(part->partnote[0].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[0].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);

            TS_ASSERT_EQUALS(part->partnote[1].status, Part::KEY_PLAYING);
            TS_ASSERT_EQUALS(part->partnote[1].note, 65);
            TS_ASSERT_EQUALS(part->partnote[1].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[1].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);

            TS_ASSERT_EQUALS(part->partnote[2].status, Part::KEY_OFF);
            TS_ASSERT_EQUALS(part->partnote[2].note, -1);
        }

        void testNoKitYesLegatoNoMono() {
            part->Ppolymode   = false;
            part->Plegatomode = true;
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);
            TS_ASSERT_EQUALS(part->partnote[0].status, Part::KEY_PLAYING);
            TS_ASSERT_EQUALS(part->partnote[0].note, 65);
            TS_ASSERT_EQUALS(part->partnote[0].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[0].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].adnote->legato.silent, false);


            TS_ASSERT_EQUALS(part->partnote[1].status, Part::KEY_PLAYING);
            TS_ASSERT_EQUALS(part->partnote[1].note, 65);
            TS_ASSERT_EQUALS(part->partnote[1].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[1].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].adnote->legato.silent, true);


            TS_ASSERT_EQUALS(part->partnote[2].status, Part::KEY_OFF);
            TS_ASSERT_EQUALS(part->partnote[2].note, -1);
        }

        void testNoKitNoLegatoYesMono() {
            part->Ppolymode   = false;
            part->Plegatomode = false;
            part->NoteOn(64, 127, 0);
            part->NoteOn(65, 127, 0);
            TS_ASSERT_EQUALS(part->partnote[0].status, Part::KEY_RELEASED);
            TS_ASSERT_EQUALS(part->partnote[0].note, 64);
            TS_ASSERT_EQUALS(part->partnote[0].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[0].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].adnote->legato.silent, false);


            TS_ASSERT_EQUALS(part->partnote[1].status, Part::KEY_PLAYING);
            TS_ASSERT_EQUALS(part->partnote[1].note, 65);
            TS_ASSERT_EQUALS(part->partnote[1].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[1].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].adnote->legato.silent, false);


            TS_ASSERT_EQUALS(part->partnote[2].status, Part::KEY_OFF);
            TS_ASSERT_EQUALS(part->partnote[2].note, -1);
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

            TS_ASSERT_EQUALS(part->partnote[0].status, Part::KEY_PLAYING);
            TS_ASSERT_EQUALS(part->partnote[0].note, 64);
            TS_ASSERT_EQUALS(part->partnote[0].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[0].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_DIFFERS(part->partnote[0].kititem[1].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].subnote, nullptr);

            TS_ASSERT_EQUALS(part->partnote[1].status, Part::KEY_PLAYING);
            TS_ASSERT_EQUALS(part->partnote[1].note, 65);
            TS_ASSERT_EQUALS(part->partnote[1].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[1].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_DIFFERS(part->partnote[1].kititem[1].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].subnote, nullptr);

            TS_ASSERT_EQUALS(part->partnote[2].status, Part::KEY_OFF);
            TS_ASSERT_EQUALS(part->partnote[2].note, -1);
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
            TS_ASSERT_EQUALS(part->partnote[0].status, Part::KEY_PLAYING);
            TS_ASSERT_EQUALS(part->partnote[0].note, 65);
            TS_ASSERT_EQUALS(part->partnote[0].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[0].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_DIFFERS(part->partnote[0].kititem[1].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].adnote->legato.silent, false);


            TS_ASSERT_EQUALS(part->partnote[1].status, Part::KEY_PLAYING);
            TS_ASSERT_EQUALS(part->partnote[1].note, 65);
            TS_ASSERT_EQUALS(part->partnote[1].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[1].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_DIFFERS(part->partnote[1].kititem[1].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].adnote->legato.silent, true);


            TS_ASSERT_EQUALS(part->partnote[2].status, Part::KEY_OFF);
            TS_ASSERT_EQUALS(part->partnote[2].note, -1);
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
            TS_ASSERT_EQUALS(part->partnote[0].status, Part::KEY_RELEASED);
            TS_ASSERT_EQUALS(part->partnote[0].note, 64);
            TS_ASSERT_EQUALS(part->partnote[0].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[0].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_DIFFERS(part->partnote[0].kititem[1].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].adnote->legato.silent, false);


            TS_ASSERT_EQUALS(part->partnote[1].status, Part::KEY_PLAYING);
            TS_ASSERT_EQUALS(part->partnote[1].note, 65);
            TS_ASSERT_EQUALS(part->partnote[1].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[1].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_DIFFERS(part->partnote[1].kititem[1].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].adnote->legato.silent, false);


            TS_ASSERT_EQUALS(part->partnote[2].status, Part::KEY_OFF);
            TS_ASSERT_EQUALS(part->partnote[2].note, -1);
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

            TS_ASSERT_EQUALS(part->partnote[0].status, Part::KEY_PLAYING);
            TS_ASSERT_EQUALS(part->partnote[0].note, 64);
            TS_ASSERT_EQUALS(part->partnote[0].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[0].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[1].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].subnote, nullptr);

            TS_ASSERT_EQUALS(part->partnote[1].status, Part::KEY_PLAYING);
            TS_ASSERT_EQUALS(part->partnote[1].note, 65);
            TS_ASSERT_EQUALS(part->partnote[1].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[1].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[1].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].subnote, nullptr);

            TS_ASSERT_EQUALS(part->partnote[2].status, Part::KEY_OFF);
            TS_ASSERT_EQUALS(part->partnote[2].note, -1);
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
            TS_ASSERT_EQUALS(part->partnote[0].status, Part::KEY_PLAYING);
            TS_ASSERT_EQUALS(part->partnote[0].note, 65);
            TS_ASSERT_EQUALS(part->partnote[0].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[0].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[1].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].adnote->legato.silent, false);


            TS_ASSERT_EQUALS(part->partnote[1].status, Part::KEY_PLAYING);
            TS_ASSERT_EQUALS(part->partnote[1].note, 65);
            TS_ASSERT_EQUALS(part->partnote[1].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[1].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[1].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].adnote->legato.silent, true);


            TS_ASSERT_EQUALS(part->partnote[2].status, Part::KEY_OFF);
            TS_ASSERT_EQUALS(part->partnote[2].note, -1);
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
            TS_ASSERT_EQUALS(part->partnote[0].status, Part::KEY_RELEASED);
            TS_ASSERT_EQUALS(part->partnote[0].note, 64);
            TS_ASSERT_EQUALS(part->partnote[0].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[0].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[1].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[0].kititem[0].adnote->legato.silent, false);


            TS_ASSERT_EQUALS(part->partnote[1].status, Part::KEY_PLAYING);
            TS_ASSERT_EQUALS(part->partnote[1].note, 65);
            TS_ASSERT_EQUALS(part->partnote[1].time, 0);
            TS_ASSERT_DIFFERS(part->partnote[1].kititem[0].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[1].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[1].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].adnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[2].subnote, nullptr);
            TS_ASSERT_EQUALS(part->partnote[1].kititem[0].adnote->legato.silent, false);


            TS_ASSERT_EQUALS(part->partnote[2].status, Part::KEY_OFF);
            TS_ASSERT_EQUALS(part->partnote[2].note, -1);
        }

        void tearDown() {
            delete part;
            delete[] denormalkillbuf;
            delete[] outL;
            delete[] outR;
            delete synth;
        }
};
