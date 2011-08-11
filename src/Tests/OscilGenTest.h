#include <cxxtest/TestSuite.h>
#include <string>
#include "../Synth/OscilGen.h"

using namespace std;

class OscilGenTest:public CxxTest::TestSuite
{
    public:
        float freq;
        float *outR, *outL;
        FFTwrapper *fft;
        OscilGen  *oscil;

        void setUp() {
            //First the sensible settings and variables that have to be set:
            SOUND_BUFFER_SIZE = 256;
            OSCIL_SIZE        = 1024;

            outL = new float[OSCIL_SIZE];
            outR = new float[OSCIL_SIZE];
            memset(outL, 0, sizeof(float) * OSCIL_SIZE);
            memset(outR, 0, sizeof(float) * OSCIL_SIZE);

            //next the bad global variables that for some reason have not been properly placed in some
            //initialization routine, but rather exist as cryptic oneliners in main.cpp:
            denormalkillbuf = new float[SOUND_BUFFER_SIZE];
            for(int i = 0; i < SOUND_BUFFER_SIZE; i++)
                denormalkillbuf[i] = 0;

            //prepare the default settings
            fft   = new FFTwrapper(OSCIL_SIZE);
            oscil = new OscilGen(fft, NULL);

            //Assert defaults [TODO]


            XMLwrapper *wrap = new XMLwrapper();
            wrap->loadXMLfile(string(SOURCE_DIR)
                              + string("/Tests/guitar-adnote.xmz"));
            TS_ASSERT(wrap->enterbranch("MASTER"));
            TS_ASSERT(wrap->enterbranch("PART", 0));
            TS_ASSERT(wrap->enterbranch("INSTRUMENT"));
            TS_ASSERT(wrap->enterbranch("INSTRUMENT_KIT"));
            TS_ASSERT(wrap->enterbranch("INSTRUMENT_KIT_ITEM", 0));
            TS_ASSERT(wrap->enterbranch("ADD_SYNTH_PARAMETERS"));
            TS_ASSERT(wrap->enterbranch("VOICE", 0));
            TS_ASSERT(wrap->enterbranch("OSCIL"));
            oscil->getfromXML(wrap);
            delete wrap;

            //verify xml was loaded [TODO]

            //lets go with.... 50! as a nice note
            const char testnote = 50;
            freq = 440.0 * pow(2.0, (testnote - 69.0) / 12.0);
        }
        
        void tearDown() {
            delete oscil;
            delete fft;
            delete[] outL;
            delete[] outR;
            delete[] denormalkillbuf;
        }

        //verifies that initialization occurs
        void testInit(void)
        {
            oscil->get(outL, freq);
        }

        void testOutput(void)
        {
            oscil->get(outL, freq);
            TS_ASSERT_DELTA(outL[23],  -0.014717, 0.0001);
            TS_ASSERT_DELTA(outL[129], -0.567502, 0.0001);
            TS_ASSERT_DELTA(outL[586], -0.030894, 0.0001);
            TS_ASSERT_DELTA(outL[1023], -0.080001, 0.0001);
        }

        void testSpectrum(void)
        {
            oscil->getspectrum(OSCIL_SIZE / 2, outR, 1);
            TS_ASSERT_DELTA(outR[0], 350.698059, 0.0001);
            TS_ASSERT_DELTA(outR[1], 228.889267, 0.0001);
            TS_ASSERT_DELTA(outR[2], 62.187931, 0.0001);
            TS_ASSERT_DELTA(outR[3], 22.295225, 0.0001);
            TS_ASSERT_DELTA(outR[4], 6.942001, 0.0001);
            TS_ASSERT_DELTA(outR[26], 0.015110, 0.0001);
            TS_ASSERT_DELTA(outR[47], 0.003425, 0.0001);
            TS_ASSERT_DELTA(outR[65], 0.001293, 0.0001);
        }
};
