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
            master->setUnknownAddressCallback(__unknownAddressCallback, this);
        }

        // TODO: eliminate static callbacks
        static void __masterChangedCallback(void* ptr, zyn::Master* m)
        {
            ((SaveOSCTest*)ptr)->_masterChangedCallback(m);
        }

        std::atomic<unsigned> unknown_addresses_count = { 0 };

        void _unknownAddressCallback(bool, rtosc::msg_t)
        {
            ++unknown_addresses_count;
        }

        static void __unknownAddressCallback(void* ptr, bool offline, rtosc::msg_t msg)
        {
            ((SaveOSCTest*)ptr)->_unknownAddressCallback(offline, msg);
        }

        void setUp() {
            // this might be set to true in the future
            // when saving will work better
            config.cfg.SaveFullXml = false;

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
            std::string savefile_content;
            int msgnext = 0, msgmax;
        } recent;
        std::mutex cb_mutex;
        using mutex_guard = std::lock_guard<std::mutex>;

        bool timeOutOperation(const char* osc_path, const char* arg1, int tries)
        {
            clock_t begin = clock(); // just for statistics

            bool ok = false;
            rtosc_arg_val_t start_time;
            rtosc_arg_val_current_time(&start_time);

            mw->transmitMsgGui(osc_path, "stT", arg1, start_time.val.t);

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
                int args = rtosc_narguments(msg);

                if(args == 3)
                {
                    assert(   !strcmp(rtosc_argument_string(msg), "stT")
                           || !strcmp(rtosc_argument_string(msg), "stF"));
                    recent.operation = msg;
                    recent.file = rtosc_argument(msg, 0).s;
                    recent.stamp = rtosc_argument(msg, 1).t;
                    recent.status = rtosc_argument(msg, 2).T;
                    recent.savefile_content.clear();
                    recent.msgmax = recent.msgnext = 0;
                }
                else
                {
                    assert(!strcmp(rtosc_argument_string(msg), "stiis"));
                    assert(rtosc_argument(msg, 0).s == recent.file);
                    assert(rtosc_argument(msg, 1).t == recent.stamp);
                    if(recent.msgmax)
                        assert(recent.msgmax == rtosc_argument(msg, 3).i);
                    else
                        recent.msgmax = rtosc_argument(msg, 3).i;
                    assert(recent.msgnext == rtosc_argument(msg, 2).i);
                    recent.savefile_content += rtosc_argument(msg, 4).s;
                    ++recent.msgnext;
                }
            }
            else if(!strcmp(msg, "/damage"))
            {
                // (ignore)
            }
            else
            {
                // parameter update triggers message to UI
                // (ignore)
            }
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
            int rval_total = EXIT_SUCCESS;
            for(int idx = 1; idx < argc && rval_total == EXIT_SUCCESS; ++idx)
            {
                const char *filename = argv[idx];
                assert(mw);
                int rval;

                //possibly not needed: load_xmz *should* clean up everything
                //mw->transmitMsgGui("/reset_master", "");

                fprintf(stderr, "Loading XML file %s...\n", filename);
                if(timeOutOperation("/load_xmz", filename, 1000))
                {
                    fputs("Saving OSC file now...\n", stderr);
                    // There is actually no need to wait for /save_osc, since
                    // we're in the "UI" thread which does the saving itself,
                    // but this gives an example how it works with remote front-ends
                    // The filename '""' will write the savefile to stdout
                    rval = timeOutOperation("/save_osc", "", 1000)
                         ? EXIT_SUCCESS
                         : EXIT_FAILURE;
                }
                else
                {
                    std::cerr << "ERROR: Could not load master file "
                              << filename << "." << std::endl;
                    rval = EXIT_FAILURE;
                }

                if(rval == EXIT_FAILURE)
                    rval_total = EXIT_FAILURE;
            }

            return rval_total;
        }


        // enable everything, cycle through all presets
        // => only preset ports and enable ports are saved
        // => all other ports are defaults and thus not saved
        int run2(int argc, char** argv)
        {
            // enable almost everything, in order to test all ports
            mw->transmitMsgGui("/part0/kit0/adpars/VoicePar0/PFilterEnabled", "T");
            mw->transmitMsgGui("/part0/kit0/adpars/VoicePar0/PFreqEnvelopeEnabled", "T");
            mw->transmitMsgGui("/part0/kit0/adpars/VoicePar0/PFreqLfoEnabled", "T");
            mw->transmitMsgGui("/part0/kit0/adpars/VoicePar0/PAAEnabled", "T");
            mw->transmitMsgGui("/part0/kit0/adpars/VoicePar0/PAmpEnvelopeEnabled", "T");
            mw->transmitMsgGui("/part0/kit0/adpars/VoicePar0/PAmpLfoEnabled", "T");
            mw->transmitMsgGui("/part0/kit0/adpars/VoicePar0/PFilterEnabled", "T");
            mw->transmitMsgGui("/part0/kit0/adpars/VoicePar0/PFilterEnvelopeEnabled", "T");
            mw->transmitMsgGui("/part0/kit0/adpars/VoicePar0/PFilterLfoEnabled", "T");
            mw->transmitMsgGui("/part0/kit0/adpars/VoicePar0/PFMEnabled", "i", 1);
            mw->transmitMsgGui("/part0/kit0/adpars/VoicePar0/PFMFreqEnvelopeEnabled", "T");
            mw->transmitMsgGui("/part0/kit0/adpars/VoicePar0/PFMAmpEnvelopeEnabled", "T");
            mw->transmitMsgGui("/part0/kit0/Psubenabled", "T");
            mw->transmitMsgGui("/part0/kit0/subpars/PBandWidthEnvelopeEnabled", "T");
            mw->transmitMsgGui("/part0/kit0/subpars/PFreqEnvelopeEnabled", "T");
            mw->transmitMsgGui("/part0/kit0/subpars/PGlobalFilterEnabled", "T");
            mw->transmitMsgGui("/part0/kit0/Ppadenabled", "T");

            // use all effects as ins fx
            mw->transmitMsgGui("/insefx0/efftype", "S", "Reverb");
            mw->transmitMsgGui("/insefx1/efftype", "S", "Phaser");
            mw->transmitMsgGui("/insefx2/efftype", "S", "Echo");
            mw->transmitMsgGui("/insefx3/efftype", "S", "Distortion");
            mw->transmitMsgGui("/insefx4/efftype", "S", "Sympathetic");
            mw->transmitMsgGui("/insefx5/efftype", "S", "DynFilter");
            mw->transmitMsgGui("/insefx6/efftype", "S", "Alienwah");
            mw->transmitMsgGui("/insefx7/efftype", "S", "EQ");
            mw->transmitMsgGui("/part0/partefx0/efftype", "S", "Chorus");
            // use all effects as sys fx (except Chorus, it does not differ)
            mw->transmitMsgGui("/sysefx0/efftype", "S", "Reverb");
            mw->transmitMsgGui("/sysefx1/efftype", "S", "Phaser");
            mw->transmitMsgGui("/sysefx2/efftype", "S", "Echo");
            mw->transmitMsgGui("/sysefx3/efftype", "S", "Distortion");

            int res = EXIT_SUCCESS;

            for (int preset = 0; preset < 18 && res == EXIT_SUCCESS; ++preset)
            {
                std::cout << "testing preset " << preset << std::endl;

                // save all insefx with preset "preset"
                char insefxstr[] = "/insefx0/preset";
                char partefxstr[] = "/part0/partefx0/preset";
                char sysefxstr[] = "/sysefx0/preset";
                int npresets_ins[] = {13, 12, 9, 6, 5, 5, 4, 2};
                int npresets_part[] = {10};
                for(; insefxstr[7] < '8'; ++insefxstr[7])
                    if(preset < npresets_ins[insefxstr[7]-'0'])
                        mw->transmitMsgGui(insefxstr, "i", preset);
                for(; partefxstr[14] < '1'; ++partefxstr[14])
                    if(preset < npresets_part[partefxstr[14]-'0'])
                        mw->transmitMsgGui(partefxstr, "i", preset);
                if(preset == 13) // for presets 13-17, test the up to 5 sysefx
                {
                    mw->transmitMsgGui("/sysefx0/efftype", "S", "Sympathetic");
                    mw->transmitMsgGui("/sysefx1/efftype", "S", "DynFilter");
                    mw->transmitMsgGui("/sysefx2/efftype", "S", "Alienwah");
                    mw->transmitMsgGui("/sysefx3/efftype", "S", "EQ");
                }
                int type_offset = (preset>=13)?4:0;
                for(; sysefxstr[7] < '4'; ++sysefxstr[7])
                    if(preset%13 < npresets_ins[sysefxstr[7]-'0'+type_offset])
                        mw->transmitMsgGui(sysefxstr, "i", preset%13);

                char filename[] = "file0";
                filename[4] += preset;
                res = timeOutOperation("/save_osc", filename, 1000)
                      ? res
                      : EXIT_FAILURE;


                int attempt;
                for(attempt = 0; attempt < 1000; ++attempt)
                {
                    mutex_guard guard(cb_mutex);
                    if(recent.msgmax &&
                       recent.msgnext > recent.msgmax)
                    {
                        break;
                    }
                    usleep(1000);
                }

                assert(attempt < 1000);
                const std::string& savefile = recent.savefile_content;
                std::cout << "Saving "
                          << (res == EXIT_SUCCESS ? "successful" : "failed")
                          << "." << std::endl;
                std::cout << "The savefile content follows" << std::endl;
                std::cout << "----8<----" << std::endl;
                std::cout << savefile << std::endl;
                std::cout << "---->8----" << std::endl;

                const char* next_line;
                for(const char* line = savefile.c_str();
                    *line && res == EXIT_SUCCESS;
                    line = next_line)
                {
                    next_line = strchr(line, '\n');
                    if (next_line) { ++next_line; } // first char of new line
                    else { next_line = line + strlen(line); } // \0 terminator

                    auto line_contains = [](const char* line, const char* next_line, const char* what) {
                        auto pos = strstr(line, what);
                        return (pos && pos < next_line);
                    };

                    if(// empty line / comment
                       line[0] == '\n' || line[0] == '%' ||
                       // accept [Ee]nabled, presets, and effect types,
                       // because we set them ourselves
                       line_contains(line, next_line, "nabled") ||
                       line_contains(line, next_line, "/preset") ||
                       line_contains(line, next_line, "/efftype") ||
                       // formants have random values:
                       line_contains(line, next_line, "/Pformants")
                       )
                    {
                        // OK
                    } else {
                        // everything else will not be OK, because this means
                        // a derivation from the default value, while we only
                        // used default values
                        // => that would mean an rDefault/rPreset does not match
                        //    what the oject really uses as default value
                        std::string bad_line(line, next_line-1);
                        std::cout << "Error: invalid rDefault/rPreset: "
                                  << bad_line
                                  << std::endl;
                        res = EXIT_FAILURE;
                    }
                }

                if(unknown_addresses_count)
                {
                    std::cout << "Error: master caught unknown addresses"
                              << std::endl;
                    res = EXIT_FAILURE;
                }
            }
            return res;
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
    printf("argc: %d\n", argc);
    int res = argc == 1 ? test.run2(argc, argv)
                        : test.run(argc, argv);
    test.stop_realtime();
    std::cerr << "Summary: " << ((res == EXIT_SUCCESS) ? "SUCCESS" : "FAILURE")
              << std::endl;
    return res;
}
