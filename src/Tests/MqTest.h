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
#include "../Containers/MultiPseudoStack.h"
using namespace std;

char *instance_name=(char*)"";

class MessageTest:public CxxTest::TestSuite
{
    public:
        MultiPseudoStack *s;
        void setUp() {
            s = new MultiPseudoStack;
        }

        void tearDown() {
            delete s;
        }

        void testBasic(void)
        {
            auto *mem = s->alloc();
            TS_ASSERT(mem);
            TS_ASSERT(mem->memory);
            TS_ASSERT(!s->read());
            s->write(mem);
            auto *mem2 = s->read();
            TS_ASSERT_EQUALS(mem, mem2);
            s->free(mem2);
        }

#define OPS 10000
        void testThreads(void)
        {
            std::thread *t[32];
            for(int i=0; i<32; ++i) {
                t[i] = new std::thread([this,i](){
                    int op=0;
                    while(op<OPS) {
                        int read = rand()%2;
                        if(read) {
                            auto *mem = s->read();
                            if(mem) {
                                //printf("r%d",i%10);
                                //printf("got: <%s>\n", mem->memory);
                                op++;
                            }
                            s->free(mem);
                        } else {
                            auto *mem = s->alloc();
                            if(mem) {
                                sprintf(mem->memory,"written by %d@op%d", i,op);
                                //printf("w%d",i%10);
                                op++;
                            }
                            s->write(mem);
                        }
                    }
                });
            }

            printf("thread started...\n");
            for(int i=0; i<32; ++i) {
                t[i]->join();
                delete t[i];
            }
            printf("thread stopped...\n");
        }
};
