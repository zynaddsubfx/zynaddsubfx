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
#include <thread>
#include <rtosc/thread-link.h>
#include <unistd.h>
#include "../Containers/MultiPseudoStack.h"
using namespace std;
using namespace zyn;

char *instance_name=(char*)"";

class MessageTest
{
    public:
        MultiQueue *s;
        void setUp() {
            s = new MultiQueue;
        }

        void tearDown() {
            delete s;
        }

        void testBasic(void)
        {
            auto *mem = s->alloc();
            TS_NON_NULL(mem);
            TS_NON_NULL(mem->memory);
            TS_ASSERT(!s->read());
            s->write(mem);
            auto *mem2 = s->read();
            assert_ptr_eq(mem, mem2,
                    "both locations are the same", __LINE__);
            s->free(mem2);
        }
        void testMed(void)
        {
            for(int i=0; i<100; ++i) {
                auto *mem = s->alloc();
                TS_NON_NULL(mem);
                TS_NON_NULL(mem->memory);
                TS_ASSERT(!s->read());
                s->write(mem);
                auto *mem2 = s->read();
                assert_ptr_eq(mem, mem2,
                        "both locations are the same", __LINE__);
                s->free(mem2);
            }
        }

#define OPS 1000
#define THREADS 8
        void testThreads(void)
        {
            uint8_t messages[OPS*THREADS];
            memset(messages, 0, sizeof(messages));
            std::thread *t[THREADS];
            for(int i=0; i<THREADS; ++i) {
                t[i] = new std::thread([this,i,&messages](){
                    int op=0;
                    while(op<OPS) {
                        int read = rand()%2;
                        if(read) {
                            auto *mem = s->read();
                            if(mem) {
                                //printf("r%d",i%10);
                                //printf("got: <%s>\n", mem->memory);
                                messages[atoi(mem->memory)]++;
                            }
                            s->free(mem);
                        } else {
                            auto *mem = s->alloc();
                            if(mem) {
                                sprintf(mem->memory,"%d written by %d@op%d", i*OPS+op,i,op);
                                //printf("w%d",i%10);
                                op++;
                            }
                            s->write(mem);
                        }
                    }
                });
            }

            printf("thread started...\n");
            for(int i=0; i<THREADS; ++i) {
                t[i]->join();
                delete t[i];
            }
            printf("thread stopped...\n");
            //read the last few
            while(1) {
                auto *mem = s->read();
                if(mem) {
                    printf("got: <%s>\n", mem->memory);
                    messages[atoi(mem->memory)]++;
                } else
                    break;
                s->free(mem);
            }

            int good = 1;
            for(int i=0; i<OPS*THREADS; ++i) {
                if(messages[i] != 1) {
                    assert(false);
                    good = 0;
                }
            }
            TS_ASSERT(good);
        }
};

int main()
{
    MessageTest test;
    RUN_TEST(testBasic);
    RUN_TEST(testMed);
    RUN_TEST(testThreads);
    return test_summary();
}
