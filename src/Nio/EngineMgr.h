#ifndef ENGINE_MGR_H
#define ENGINE_MGR_H

#include <list>
#include <string>
#include "Engine.h"


class MidiIn;
class AudioOut;
class OutMgr;
/**Container/Owner of the long lived Engines*/
class EngineMgr
{
    public:
        EngineMgr();
        ~EngineMgr();

        /**Gets requested engine
         * @param name case unsensitive name of engine
         * @return pointer to Engine or NULL
         */
        Engine *getEng(std::string name);

    private:
        std::list<Engine *> engines;

        Engine *defaultEng;/**<The default output*/

        //Engine Manager user
        //[if there is another 'user, just make this class a struct]
        friend class OutMgr;
        friend class NioUI;
};

extern EngineMgr *sysEngine;
#endif

