#include <cassert>
#include <dirent.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <ctime>
#include <unistd.h>
#include <rtosc/thread-link.h>
#include <rtosc/rtosc-time.h>

#ifndef ZYN_GIT_WORKTREE
    #include <git2.h>
#endif

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
            mw->setUiCallback(0, _uiCallback, this);
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
            int msgnext = 0, msgmax = -1;
        } recent;
        std::mutex cb_mutex;
        using mutex_guard = std::lock_guard<std::mutex>;

        bool timeOutOperation(const char* osc_path, const char* arg1, int tries, int prependArg = -1)
        {
            clock_t begin = clock(); // just for statistics

            bool ok = false;
            rtosc_arg_val_t start_time;
            rtosc_arg_val_current_time(&start_time);

            if(prependArg == -1)
            {
                mw->transmitMsgGui(0, osc_path, "stT", arg1, start_time.val.t);
            } else {
                mw->transmitMsgGui(0, osc_path, "istT", prependArg, arg1, start_time.val.t);
            }

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
            if(!strcmp(msg, "/save_osc") || !strcmp(msg, "/load_xmz") || !strcmp(msg, "/load_xiz"))
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
                    recent.msgnext = 0;
                    recent.msgmax = -1;
                }
                else
                {
                    assert(!strcmp(rtosc_argument_string(msg), "stiis"));
                    assert(rtosc_argument(msg, 0).s == recent.file);
                    assert(rtosc_argument(msg, 1).t == recent.stamp);
                    if(recent.msgmax == -1)
                        recent.msgmax = rtosc_argument(msg, 3).i;
                    else
                        assert(recent.msgmax == rtosc_argument(msg, 3).i);
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

        void wait_for_message()
        {
            int attempt;
            for(attempt = 0; attempt < 1000; ++attempt)
            {
                mutex_guard guard(cb_mutex);
                if((recent.msgmax != -1) &&
                   recent.msgnext > recent.msgmax)
                {
                    break;
                }
                usleep(1000);
            }
            assert(attempt < 1000);
        }

        void dump_savefile(int res)
        {
            const std::string& savefile = recent.savefile_content;
            std::cout << "Saving "
                      << (res == EXIT_SUCCESS ? "successful" : "failed")
                      << "." << std::endl;
            std::cout << "The savefile content follows" << std::endl;
            std::cout << "----8<----" << std::endl;
            std::cout << savefile << std::endl;
            std::cout << "---->8----" << std::endl;
        }

    public:
        SaveOSCTest() { setUp(); }
        ~SaveOSCTest() { tearDown(); }

        static void _uiCallback(void* ptr, const char* msg)
        {
            ((SaveOSCTest*)ptr)->uiCallback(msg);
        }

        int test_files(const std::vector<std::string>& filenames)
        {
            int rval_total = EXIT_SUCCESS;
            for(std::size_t idx = 0; idx < filenames.size() && rval_total == EXIT_SUCCESS; ++idx)
            {
                const std::string& filename = filenames[idx];
                assert(mw);
                int rval;

                bool load_ok;
                if (strstr(filename.c_str(), ".xiz") ==
                    filename.c_str() + filename.length() - 4)
                {
                    mw->transmitMsgGui(0, "/reset_master", "");
                    fprintf(stderr, "Loading XIZ file %s...\n", filename.c_str());
                    load_ok = timeOutOperation("/load_xiz", filename.c_str(), 1000, 0);
                }
                else {
                    fprintf(stderr, "Loading XMZ file %s...\n", filename.c_str());
                    load_ok = timeOutOperation("/load_xmz", filename.c_str(), 1000);
                }
                mw->discardAllbToUButHandleFree();

                if(load_ok)
                {
                    fputs("Saving OSC file now...\n", stderr);
                    // There is actually no need to wait for /save_osc, since
                    // we're in the "UI" thread which does the saving itself,
                    // but this gives an example how it works with remote front-ends
                    // The filename '""' will write the savefile to stdout
                    rval = timeOutOperation("/save_osc", "", 1000)
                         ? EXIT_SUCCESS
                         : EXIT_FAILURE;
                    wait_for_message();
                    mw->discardAllbToUButHandleFree();
                    dump_savefile(rval);
                }
                else
                {
                    std::cerr << "ERROR: Could not load XML file "
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
        int test_presets()
        {
            // enable almost everything, in order to test all ports
            mw->transmitMsgGui(0, "/part0/kit0/adpars/VoicePar0/PFilterEnabled", "T");
            mw->transmitMsgGui(0, "/part0/kit0/adpars/VoicePar0/PFreqEnvelopeEnabled", "T");
            mw->transmitMsgGui(0, "/part0/kit0/adpars/VoicePar0/PFreqLfoEnabled", "T");
            mw->transmitMsgGui(0, "/part0/kit0/adpars/VoicePar0/PAAEnabled", "T");
            mw->transmitMsgGui(0, "/part0/kit0/adpars/VoicePar0/PAmpEnvelopeEnabled", "T");
            mw->transmitMsgGui(0, "/part0/kit0/adpars/VoicePar0/PAmpLfoEnabled", "T");
            mw->transmitMsgGui(0, "/part0/kit0/adpars/VoicePar0/PFilterEnabled", "T");
            mw->transmitMsgGui(0, "/part0/kit0/adpars/VoicePar0/PFilterEnvelopeEnabled", "T");
            mw->transmitMsgGui(0, "/part0/kit0/adpars/VoicePar0/PFilterLfoEnabled", "T");
            mw->transmitMsgGui(0, "/part0/kit0/adpars/VoicePar0/PFMEnabled", "i", 1);
            mw->transmitMsgGui(0, "/part0/kit0/adpars/VoicePar0/PFMFreqEnvelopeEnabled", "T");
            mw->transmitMsgGui(0, "/part0/kit0/adpars/VoicePar0/PFMAmpEnvelopeEnabled", "T");
            mw->transmitMsgGui(0, "/part0/kit0/Psubenabled", "T");
            mw->transmitMsgGui(0, "/part0/kit0/subpars/PBandWidthEnvelopeEnabled", "T");
            mw->transmitMsgGui(0, "/part0/kit0/subpars/PFreqEnvelopeEnabled", "T");
            mw->transmitMsgGui(0, "/part0/kit0/subpars/PGlobalFilterEnabled", "T");
            mw->transmitMsgGui(0, "/part0/kit0/Ppadenabled", "T");

            // use all effects as ins fx
            mw->transmitMsgGui(0, "/insefx0/efftype", "S", "Reverb");
            mw->transmitMsgGui(0, "/insefx1/efftype", "S", "Phaser");
            mw->transmitMsgGui(0, "/insefx2/efftype", "S", "Echo");
            mw->transmitMsgGui(0, "/insefx3/efftype", "S", "Distortion");
            mw->transmitMsgGui(0, "/insefx4/efftype", "S", "Sympathetic");
            mw->transmitMsgGui(0, "/insefx5/efftype", "S", "DynFilter");
            mw->transmitMsgGui(0, "/insefx6/efftype", "S", "Alienwah");
            mw->transmitMsgGui(0, "/insefx7/efftype", "S", "EQ");
            mw->transmitMsgGui(0, "/part0/partefx0/efftype", "S", "Chorus");
            mw->transmitMsgGui(0, "/part0/partefx1/efftype", "S", "Reverse");
            // use all effects as sys fx (except Chorus, it does not differ)
            mw->transmitMsgGui(0, "/sysefx0/efftype", "S", "Reverb");
            mw->transmitMsgGui(0, "/sysefx1/efftype", "S", "Phaser");
            mw->transmitMsgGui(0, "/sysefx2/efftype", "S", "Echo");
            mw->transmitMsgGui(0, "/sysefx3/efftype", "S", "Distortion");

            int res = EXIT_SUCCESS;

            for (int preset = 0; preset < 18 && res == EXIT_SUCCESS; ++preset)
            {
                std::cout << "testing preset " << preset << std::endl;

                // save all insefx with preset "preset"
                char insefxstr[] = "/insefx0/preset";
                char partefxstr[] = "/part0/partefx0/preset";
                char sysefxstr[] = "/sysefx0/preset";
                int npresets_ins[] = {13, 12, 9, 6, 5, 5, 4, 2};
                int npresets_part[] = {12, 3};
                for(; insefxstr[7] < '0' + (char)(sizeof(npresets_ins) / sizeof(npresets_ins[0])); ++insefxstr[7])
                    if(preset < npresets_ins[insefxstr[7]-'0'])
                        mw->transmitMsgGui(0, insefxstr, "i", preset);  // "/insefx?/preset ?"
                for(; partefxstr[14] < '0' + (char)(sizeof(npresets_part) / sizeof(npresets_part[0])); ++partefxstr[14])
                    if(preset < npresets_part[partefxstr[14]-'0'])
                        mw->transmitMsgGui(0, partefxstr, "i", preset);  // "/part0/partefx?/preset ?"
                if(preset == 13) // for presets 13-17, test the up to 5 sysefx
                {
                    mw->transmitMsgGui(0, "/sysefx0/efftype", "S", "Sympathetic");
                    mw->transmitMsgGui(0, "/sysefx1/efftype", "S", "DynFilter");
                    mw->transmitMsgGui(0, "/sysefx2/efftype", "S", "Alienwah");
                    mw->transmitMsgGui(0, "/sysefx3/efftype", "S", "EQ");
                }
                int type_offset = (preset>=13)?4:0;
                for(; sysefxstr[7] < '4'; ++sysefxstr[7])
                    if(preset%13 < npresets_ins[sysefxstr[7]-'0'+type_offset])
                        mw->transmitMsgGui(0, sysefxstr, "i", preset%13);  // "/sysefx?/preset ?"

                char filename[] = "file0";
                filename[4] += preset;
                res = timeOutOperation("/save_osc", filename, 1000)
                      ? res
                      : EXIT_FAILURE;


                wait_for_message();
                dump_savefile(res);

                const std::string& savefile = recent.savefile_content;
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

        static void findfiles(std::string dirname, std::vector<std::string>& all_files)
        {
            if(dirname.back() != '/' && dirname.back() != '\\')
                dirname += '/';

            DIR *dir = opendir(dirname.c_str());
            if(dir == NULL)
                return;

            struct dirent *fn;
            while((fn = readdir(dir))) {
                const char *filename = fn->d_name;

                if(fn->d_type == DT_DIR) {
                    if(strcmp(filename, ".") && strcmp(filename, ".."))
                        findfiles(dirname + filename, all_files);
                }
                else
                {
                    std::size_t len = strlen(filename);
                    //check for extension
                    if(   (strstr(filename, ".xiz") == filename + len - 4)
                        || (strstr(filename, ".xmz") == filename + len - 4))
                    {
                        all_files.push_back(dirname + filename);
                    }
                }
            }

            closedir(dir);
        }

#ifndef ZYN_GIT_WORKTREE
        static std::string get_git_worktree()
        {
            git_libgit2_init();

            git_buf root_path = {0, 0, 0};
            git_repository_discover(&root_path, ".", 0, NULL);

            git_repository *repo = NULL;
            git_repository_open(&repo, root_path.ptr);

            std::string toplevel = git_repository_workdir(repo);

            git_repository_free(repo);
            git_buf_free(&root_path);
            git_libgit2_shutdown();

            return toplevel;
        }
#endif

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

    std::vector<std::string> all_files;
    if(argc == 1)
    {
        // leave vector empty - test presets
    } else if(!strcmp(argv[1], "test-all")) {
        std::string zyn_git_worktree;
#ifdef ZYN_GIT_WORKTREE
        zyn_git_worktree = ZYN_GIT_WORKTREE;
#else
        zyn_git_worktree = SaveOSCTest::get_git_worktree();
#endif
        SaveOSCTest::findfiles(zyn_git_worktree, all_files);
    } else {
        all_files.reserve(argc-1);
        for(int idx = 1; idx < argc; ++idx)
            all_files.push_back(argv[idx]);
    }


    int res = all_files.size() == 0 ? test.test_presets()
                                    : test.test_files(all_files);
    test.stop_realtime();
    std::cerr << "Summary: " << ((res == EXIT_SUCCESS) ? "SUCCESS" : "FAILURE")
              << std::endl;
    return res;
}
