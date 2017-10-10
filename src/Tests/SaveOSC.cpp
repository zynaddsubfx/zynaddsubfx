#include <cassert>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <rtosc/thread-link.h>

#include <cxxtest/TestSuite.h>

#include "../Misc/Master.h"
#include "../Misc/MiddleWare.h"
#include "../UI/NSM.H"

// for linking purposes only:
NSM_Client *nsm = 0;
zyn::MiddleWare *middleware = 0;

char *instance_name=(char*)"";

// TODO: Check if rNoWalk is really needed

class SaveOSCTest
{

        void _masterChangedCallback(zyn::Master* m)
        {
            /*
                Note: This message will appear 4 times:
                 * Once at startup (changing from nil)
                 * Once after the loading
                 * Twice for the temporary exchange during saving
             */
            printf("Changing master from %p (%p) to %p...\n", master, &master, m);
            master = m;
            master->setMasterChangedCallback(__masterChangedCallback, this);
        }

        static void __masterChangedCallback(void* ptr, zyn::Master* m)
        {
            ((SaveOSCTest*)ptr)->_masterChangedCallback(m);
        }

        void setUp() {
            synth = new zyn::SYNTH_T;
            synth->buffersize = 256;
            synth->samplerate = 48000;
            synth->alias();

            mw = new zyn::MiddleWare(std::move(*synth), &config);
            _masterChangedCallback(mw->spawnMaster());
            realtime = nullptr;
        }

        void tearDown() {
            printf("Master at the end: %p\n", master);
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

            assert(mw);
            mw->transmitMsg("/load_xmz", "s", filename);
            sleep(1); // TODO: Poll to find out if+when loading was finished
/*            if(tmp < 0) {
                std::cerr << "ERROR: Could not load master file " << filename
                     << "." << std::endl;
                exit(1);
            }*/

            fputs("Saving OSC file now...\n", stderr);

            mw->transmitMsg("/save_osc", "s", "");
            sleep(1);

            return EXIT_SUCCESS; // TODO: how to check load and save success?
        }


        void start_realtime(void)
        {
            do_exit = false;

            realtime = new std::thread([this](){
                while(!do_exit)
                {
                    if(!master->uToB->hasNext()) {
                        if(do_exit)
                            break;

                        usleep(500);
                        continue;
                    }
                    const char *msg = master->uToB->read();
                    printf("Master %p: handling <%s>\n", master, msg);
                    master->applyOscEvent(msg, false);
                }});
        }

        void stop_realtime(void)
        {
            do_exit = true;

            realtime->join();
            delete realtime;
            realtime = NULL;
        }

    private:
        zyn::Config config;
        zyn::SYNTH_T* synth;
        zyn::Master* master = NULL;
        zyn::MiddleWare* mw;
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
