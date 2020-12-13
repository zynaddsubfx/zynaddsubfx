#ifndef MSGPARSETEST_H
#define MSGPARSETEST_H

#include <cxxtest/TestSuite.h>
#include "../Misc/MsgParsing.h"

class PluginTest:public CxxTest::TestSuite
{
    public:
        void setUp() {}

        void tearDown() {}

        void testExtracting() {
            std::size_t res;
            int part, kit, vc;
            bool isFm;

            // test a full string with OscilSmp
            res = zyn::idsFromMsg("/part1/kit2/adpars/VoicePar3/OscilSmp", &part, &kit, &vc, &isFm);
            TS_ASSERT(res);
            TS_ASSERT_EQUALS(part, 1);
            TS_ASSERT_EQUALS(kit, 2);
            TS_ASSERT_EQUALS(vc, 3);
            TS_ASSERT(!isFm);
            // translate back into string
            std::string str = zyn::buildVoiceParMsg(&part, &kit, &vc, &isFm);
            TS_ASSERT_EQUALS(str, "/part1/kit2/adpars/VoicePar3/OscilSmp");

            // same with FMSmp
            res = zyn::idsFromMsg("/part11/kit12/adpars/VoicePar13/FMSmp", &part, &kit, &vc, &isFm);
            TS_ASSERT(res);
            TS_ASSERT_EQUALS(part, 11);
            TS_ASSERT_EQUALS(kit, 12);
            TS_ASSERT_EQUALS(vc, 13);
            TS_ASSERT(isFm);
            // translate back into string
            str = zyn::buildVoiceParMsg(&part, &kit, &vc, &isFm);
            TS_ASSERT_EQUALS(str, "/part11/kit12/adpars/VoicePar13/FMSmp");

            // check return values
            TS_ASSERT(!zyn::idsFromMsg("/part", &part, &kit, nullptr));
            TS_ASSERT(!zyn::idsFromMsg("/part1", &part, &kit, nullptr));
            TS_ASSERT(!zyn::idsFromMsg("/part1/kit", &part, &kit, nullptr));
            TS_ASSERT(zyn::idsFromMsg("/part1/kit2", &part, &kit, nullptr));
            TS_ASSERT(!zyn::idsFromMsg("/part1/kit2", &part, &kit, &vc));
            TS_ASSERT(!zyn::idsFromMsg("/part1/kit2/adpars/", &part, &kit, &vc));
            TS_ASSERT(!zyn::idsFromMsg("/part1/kit2/adpars/", &part, &kit, &vc));
            TS_ASSERT(!zyn::idsFromMsg("/part1/kit2/adpars/VoicePar", &part, &kit, &vc));
            TS_ASSERT(!zyn::idsFromMsg("/part1/kit2/adpars/VoicePar/", &part, &kit, &vc));
            TS_ASSERT(zyn::idsFromMsg("/part1/kit2/adpars/VoicePar0/", &part, &kit, &vc));
            TS_ASSERT(zyn::idsFromMsg("/part1/kit2/adpars/VoicePar0/XXX", &part, &kit, &vc));
            TS_ASSERT(!zyn::idsFromMsg("/part1/kit2/adpars/VoicePar0/XXX", &part, &kit, &vc, &isFm));
            TS_ASSERT(zyn::idsFromMsg("/part1/kit2/adpars/VoicePar0/OscilSmp", &part, &kit, &vc, &isFm));
            TS_ASSERT(zyn::idsFromMsg("/part1/kit2/adpars/VoicePar0/FMSmp", &part, &kit, &vc, &isFm));
        }
};

#endif // MSGPARSETEST_H
