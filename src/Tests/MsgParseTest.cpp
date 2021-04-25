#include "test-suite.h"
#include "../Misc/MsgParsing.h"

class PluginTest
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
            TS_ASSERT_EQUAL_INT(part, 1);
            TS_ASSERT_EQUAL_INT(kit, 2);
            TS_ASSERT_EQUAL_INT(vc, 3);
            TS_ASSERT(!isFm);
            // translate back into string
            std::string str = zyn::buildVoiceParMsg(&part, &kit, &vc, &isFm);
            TS_ASSERT_EQUAL_STR("/part1/kit2/adpars/VoicePar3/OscilSmp", str.c_str());

            // same with FMSmp
            res = zyn::idsFromMsg("/part11/kit12/adpars/VoicePar13/FMSmp", &part, &kit, &vc, &isFm);
            TS_ASSERT(res);
            TS_ASSERT_EQUAL_INT(part, 11);
            TS_ASSERT_EQUAL_INT(kit, 12);
            TS_ASSERT_EQUAL_INT(vc, 13);
            TS_ASSERT(isFm);
            // translate back into string
            str = zyn::buildVoiceParMsg(&part, &kit, &vc, &isFm);
            TS_ASSERT_EQUAL_STR("/part11/kit12/adpars/VoicePar13/FMSmp", str.c_str());

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

int main()
{
    PluginTest test;
    RUN_TEST(testExtracting);
    return test_summary();
}
