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
        void tick(void);

        static void preparePadSynth(const char *, class PADnoteParameters *){};
    private:
        struct MiddleWareImpl *impl;
};
