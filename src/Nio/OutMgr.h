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
#include <semaphore.h>


class AudioOut;
class Master;
class OutMgr
{
    public:
        OutMgr(Master *nmaster);
        ~OutMgr();

        /**Adds audio output out.*/
        void add(AudioOut *out){};
        /**Removes given audio output engine*/
        void remove(AudioOut *out){};

        /**Execute a tick*/
        const Stereo<REALTYPE *> tick(unsigned int frameSize);

        /**Request a new set of samples
         * @param n number of requested samples (defaults to 1)
         * @return -1 for locking issues 0 for valid request*/
        void requestSamples(unsigned int n=1);

        /**Gets requested driver
         * @param name case unsensitive name of driver
         * @return pointer to Audio Out or NULL
         */
        AudioOut *getOut(std::string name);

        /**Gets the name of the first running driver
         * @return if no running output, "" is returned
         */
        std::string getDriver() const;

        void run();

        bool setSink(std::string name);

        std::string getSink() const;
    private:
        void addSmps(REALTYPE *l, REALTYPE *r);
        int storedSmps() const {return priBuffCurrent.l() - priBuf.l();};
        void makeStale(unsigned int size);
        void removeStaleSmps();
        //should hold outputs here that exist for the life of the OutMgr
        AudioOut *defaultOut;/**<The default output*/

        AudioOut *currentOut;

        //should hold short lived, externally controlled Outputs (eg WavEngine)
        //[needs mutex]
        //std::list<AudioOut *> unmanagedOuts;
        //mutable pthread_mutex_t mutex;
        sem_t requested;

        //information on current active primary driver
        //sample rate
        //frame size

        /**Buffer*/
        Stereo<REALTYPE *> priBuf; //buffer for primary drivers
        Stereo<REALTYPE *> priBuffCurrent; //current array accessor
        Stereo<Sample> smps;
        REALTYPE *outl;
        REALTYPE *outr;
        Master *master;

        int stales;
};

extern OutMgr *sysOut;
#endif

