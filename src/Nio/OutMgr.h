#ifndef OUTMGR_H
#define OUTMGR_H

#include "../globals.h"
#include "../Misc/Stereo.h"
#include "../Misc/Atomic.h"
#include "../Samples/Sample.h"
#include <list>
#include <map>
#include <string>
#include <pthread.h>


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
         * @param n number of requested samples (defaults to 1)
         * @return -1 for locking issues 0 for valid request*/
        int requestSamples(unsigned int n=1);

        /**Return the number of building samples*/
        int getRunning();

        void run();

        /**Gets requested driver
         * @param name case unsensitive name of driver
         * @return pointer to Audio Out or NULL
         */
        AudioOut *getOut(std::string name);

        void *outputThread();
    private:
        bool running;
        bool init;

        //should hold outputs here that exist for the life of the OutMgr
        std::map<std::string,AudioOut *> managedOuts;
        AudioOut *defaultOut;/**<The default output*/

        //should hold short lived, externally controlled Outputs (eg WavEngine)
        //[needs mutex]
        std::list<AudioOut *> unmanagedOuts;
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

