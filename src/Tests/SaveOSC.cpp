#include <cassert>
#include <thread>
#include <iostream>
#include <unistd.h>

#include <cxxtest/TestSuite.h>

#include "../Misc/Master.h"
#include "../Misc/MiddleWare.h"
#include "../UI/NSM.H"

// for linking purposes only:
NSM_Client *nsm = 0;
zyn::MiddleWare *middleware = 0;

char *instance_name=(char*)"";

// Middleware is not required, since all ports requiring MiddleWare use the
// rNoWalk macro. If you still want to enable it, uncomment this:
// #define RUN_MIDDLEWARE

class SaveOSCTest
{
        void setUp() {
            synth = new zyn::SYNTH_T;
            synth->buffersize = 256;
            synth->samplerate = 48000;
            synth->alias();

            mw = new zyn::MiddleWare(std::move(*synth), &config);
            master = mw->spawnMaster();
            realtime = nullptr;
        }

        void tearDown() {
            delete mw;
            delete synth;
        }

    public:
        SaveOSCTest() { setUp(); }
        ~SaveOSCTest() { tearDown(); }

        int run(int argc, char** argv)
        {
            assert(argc == 2);
            const char *filename = argv[1];

            int tmp = master->loadXML(filename);
            if(tmp < 0) {
                std::cerr << "ERROR: Could not load master file " << filename
                     << "." << std::endl;
                exit(1);
            }

            assert(master);  
            return (master->saveOSC(NULL) == 0) ? 0 : 1;
        }


        void start_realtime(void)
        {
            do_exit = false;
#ifdef RUN_MIDDLEWARE
            realtime = new std::thread([this](){
                while(!do_exit)
                {
                    /*while(bToU->hasNext()) {
                        const char *rtmsg = bToU->read();
                        bToUhandle(rtmsg);
                    }*/
                    mw->tick();
                    usleep(500);
                }});
#endif
        }
        void stop_realtime(void)
        {
            do_exit = true;
#ifdef RUN_MIDDLEWARE
            realtime->join();
            delete realtime;
            realtime = NULL;
#endif
        }

    private:
        zyn::Config config;
        zyn::SYNTH_T* synth;
        zyn::MiddleWare* mw;
        zyn::Master* master;
        std::thread* realtime;
        bool do_exit;
};

int main(int argc, char** argv)
{
    SaveOSCTest test;
    test.start_realtime();
    int res = test.run(argc, argv);
    test.stop_realtime();
    return res;
}
