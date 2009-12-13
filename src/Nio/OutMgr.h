#ifndef OUTMGR_H
#define OUTMGR_H

#include "../globals.h"
#include "../Misc/Stereo.h"
#include "../Misc/Atomic.h"
#include "../Samples/Sample.h"
//#include "../Misc/Master.h"
#include <list>
#include <pthread.h>

//typedef enum
//{
//    JACK_OUTPUT;
//    ALSA_OUTPUT
//    OSS_OUTPUT;
//    WINDOWS_OUTPUT;
//    WAV_OUTPUT;
//} outputDriver;

class AudioOut;
class Master;
class OutMgr
{
    public:
        OutMgr(Master *nmaster);
        ~OutMgr();
        /**Adds audio output out.
         * @return -1 for error 0 otherwise*/
        void add(AudioOut *out);
        /**Removes given audio output engine
         * @return -1 for error 0 otherwise*/
        void remove(AudioOut *out);
        /**Request a new set of samples
         * @return -1 for locking issues 0 for valid request*/
        int requestSamples();
        /**Return the number of building samples*/
        int getRunning();
        /**Enables one instance of given driver*/
        //int enable(outputDriver out);
        /**Disables all instances of given driver*/
        //int disable(outputDriver out);
        void run();

        void *outputThread();
    private:
        bool running;
        bool init;

        std::list<AudioOut *> outs;
        mutable pthread_mutex_t mutex;

        pthread_mutex_t processing;

        pthread_t outThread;
        pthread_cond_t needsProcess;
        Atomic<int> numRequests;
        /**for closing*/
        pthread_mutex_t close_m;
        pthread_cond_t close_cond;
        /**Buffer*/
        Stereo<Sample> smps;
        REALTYPE *outl;
        REALTYPE *outr;
        Master *master;
};

extern OutMgr *sysOut;
#endif

