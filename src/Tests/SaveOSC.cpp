#include <cassert>
#include <thread>
#include <mutex>
#include <iostream>
#include <ctime>
#include <unistd.h>
#include <rtosc/thread-link.h>
#include <rtosc/rtosc-time.h>

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
#ifdef SAVE_OSC_DEBUG
            printf("Changing master from %p (%p) to %p...\n", master, &master, m);
#endif
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
            mw->setUiCallback(_uiCallback, this);
            _masterChangedCallback(mw->spawnMaster());
            realtime = nullptr;
        }

        void tearDown() {
#ifdef SAVE_OSC_DEBUG
            printf("Master at the end: %p\n", master);
#endif
            delete mw;
            delete synth;
        }

        struct {
            std::string operation;
            std::string file;
            uint64_t stamp;
            bool status;
        } recent;
        std::mutex cb_mutex;
        using mutex_guard = std::lock_guard<std::mutex>;

        bool timeOutOperation(const char* osc_path, const char* arg1, int tries)
        {
            clock_t begin = clock(); // just for statistics

            bool ok = false;
            rtosc_arg_val_t start_time;
            rtosc_arg_val_current_time(&start_time);

            mw->transmitMsg(osc_path, "st", arg1, start_time.val.t);

            int attempt;
            for(attempt = 0; attempt < tries; ++attempt)
            {
                mutex_guard guard(cb_mutex);
                if(recent.stamp == start_time.val.t &&
                   recent.operation == osc_path &&
                   recent.file == arg1)
                {
                    ok = recent.status;
                    break;
                }
                usleep(1000);
            }

            // statistics:
            clock_t end = clock();
            double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

            fprintf(stderr, "Action %s finished after %lf ms,\n"
                            "    with a timeout of <%d ms (%s)\n",
                    osc_path, elapsed_secs,
                    attempt+1,
                    attempt == tries ? "timeout"
                                     : ok ? "success" : "failure");

            return ok && (attempt != tries);
        }

        void uiCallback(const char* msg)
        {
            if(!strcmp(msg, "/save_osc") || !strcmp(msg, "/load_xmz"))
            {
                mutex_guard guard(cb_mutex);
#ifdef SAVE_OSC_DEBUG
                fprintf(stderr, "Received message \"%s\".\n", msg);
#endif
                recent.operation = msg;
                recent.file = rtosc_argument(msg, 0).s;
                recent.stamp = rtosc_argument(msg, 1).t;
                recent.status = rtosc_argument(msg, 2).T;
            }
            else if(!strcmp(msg, "/damage"))
            {
                // (ignore)
            }
            else
                fprintf(stderr, "Unknown message \"%s\", ignoring...\n", msg);
        }

    public:
        SaveOSCTest() { setUp(); }
        ~SaveOSCTest() { tearDown(); }

        static void _uiCallback(void* ptr, const char* msg)
        {
            ((SaveOSCTest*)ptr)->uiCallback(msg);
        }

        int run(int argc, char** argv)
        {
            assert(argc == 2);
            const char *filename = argv[1];
            assert(mw);
            int rval;

            fputs("Loading XML file...\n", stderr);
            if(timeOutOperation("/load_xmz", filename, 1000))
            {
                fputs("Saving OSC file now...\n", stderr);
                // There is actually no need to wait for /save_osc, since
                // we're in the "UI" thread which does the saving itself,
                // but this gives an example how it works with remote fron-ends
                // The filename '""' will write the savefile to stdout
                rval = timeOutOperation("/save_osc", "", 1000)
                     ? EXIT_SUCCESS
                     : EXIT_FAILURE;
            }
            else
            {
                std::cerr << "ERROR: Could not load master file " << filename
                     << "." << std::endl;
                rval = EXIT_FAILURE;
            }

            return rval;
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
#ifdef SAVE_OSC_DEBUG
                    printf("Master %p: handling <%s>\n", master, msg);
#endif
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
    std::cerr << "Summary: " << ((res == EXIT_SUCCESS) ? "SUCCESS" : "FAILURE")
              << std::endl;
    return res;
}
