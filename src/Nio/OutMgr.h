#ifndef OUTMGR_H
#define OUTMGR_H

#include "../globals.h"
#include "../Misc/Stereo.h"
#include "../Samples/Sample.h" //deprecated 
#include <list>
#include <string>
#include <semaphore.h>


class AudioOut;
class WavEngine;
class Master;
class OutMgr
{
    public:
        OutMgr(Master *nmaster);
        ~OutMgr();

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
         * Deprecated
         * @return if no running output, "" is returned
         */
        std::string getDriver() const;

        bool setSink(std::string name);

        std::string getSink() const;

        WavEngine *wave;     /**<The Wave Recorder*/
        friend class EngineMgr;
    private:
        void addSmps(REALTYPE *l, REALTYPE *r);
        int  storedSmps() const {return priBuffCurrent.l() - priBuf.l();};
        void removeStaleSmps();

        AudioOut *currentOut;/**<The current output driver*/

        sem_t requested;

        /**Buffer*/
        Stereo<REALTYPE *> priBuf;          //buffer for primary drivers
        Stereo<REALTYPE *> priBuffCurrent; //current array accessor
        Stereo<Sample> smps; /**Deprecated TODO Remove*/

        REALTYPE *outl;
        REALTYPE *outr;
        Master *master;

        int stales;
};

extern OutMgr *sysOut;
#endif

