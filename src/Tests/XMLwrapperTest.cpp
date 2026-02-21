/*
  ZynAddSubFX - a software synthesizer

  XMLwrapperTest.h - CxxTest for Misc/XMLwrapper
  Copyright (C) 2009-2009 Mark McCurry
  Author: Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#include "test-suite.h"
#include "../Misc/XMLwrapper.h"
#include <string>
#include "../globals.h"
using namespace std;
using namespace zyn;

SYNTH_T *synth;

class XMLwrapperTest
{
    public:
        void setUp() {
            xmla = new XMLwrapper;
            xmlb = new XMLwrapper;
        }


        void testAddPar() {
            xmla->addpar("my Pa*_ramet@er", 75);
            TS_ASSERT_EQUAL_INT(xmla->getpar("my Pa*_ramet@er", 0, -200, 200), 75);
        }

        //here to verify that no leaks occur
        void testLoad() {
            string location = string(SOURCE_DIR) + string(
                "/Tests/guitar-adnote.xmz");
            xmla->loadXMLfile(location);
        }

        void testAnotherLoad()
        {
            string dat =
                "\n<?xml version=\"1.0f\" encoding=\"UTF-8\"?>\n\
<!DOCTYPE ZynAddSubFX-data>\n\
<ZynAddSubFX-data version-major=\"2\" version-minor=\"4\"\n\
version-revision=\"1\" ZynAddSubFX-author=\"Nasca Octavian Paul\">\n\
</ZynAddSubFX-data>\n";
            xmlb->putXMLdata(dat.c_str());
        }

        void tearDown() {
            delete xmla;
            delete xmlb;
        }


    private:
        XMLwrapper *xmla;
        XMLwrapper *xmlb;
};

int main()
{
    XMLwrapperTest test;
    RUN_TEST(testAddPar);
    RUN_TEST(testLoad);
    RUN_TEST(testAnotherLoad);
    return test_summary();
}

