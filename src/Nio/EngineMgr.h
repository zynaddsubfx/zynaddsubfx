#ifndef ENGINE_MGR_H
#define ENGINE_MGR_H

#include <list>
#include <string>
#include "Engine.h"


class MidiIn;
class AudioOut;
class OutMgr;
/**Container/Owner of the long lived Engines*/
struct EngineMgr
{
        EngineMgr();
        ~EngineMgr();

        /**Gets requested engine
         * @param name case unsensitive name of engine
         * @return pointer to Engine or NULL
         */
        Engine *getEng(std::string name);

        /**Stop all engines*/
        void stop();

        std::list<Engine *> engines;

        Engine *defaultEng;/**<The default output*/
};

extern EngineMgr *sysEngine;
#endif

