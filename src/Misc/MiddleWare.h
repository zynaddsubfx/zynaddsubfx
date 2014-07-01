#pragma once

//Link between realtime and non-realtime layers
class MiddleWare
{
    public:
        MiddleWare(void);
        ~MiddleWare(void);
        //return internal master pointer
        class Master *spawnMaster(void);
        class Fl_Osc_Interface *spawnUiApi(void);
        void setUiCallback(void(*cb)(void*,const char *),void *ui);
        void setIdleCallback(void(*cb)(void));
        void tick(void);
        void pendingSetProgram(int part);

        static void preparePadSynth(const char *, class PADnoteParameters *){};
    private:
        struct MiddleWareImpl *impl;
};
