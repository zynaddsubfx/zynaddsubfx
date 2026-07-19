#ifndef PITCHDROP_H
#define PITCHDROP_H

#include "Effect.h"
#include "EffectLFO.h"

namespace zyn {

class CombFilterBank;

class PitchDrop final:public Effect
{
    public:
        PitchDrop(EffectParams pars);
        ~PitchDrop();
        void out(const Stereo<float *> &smp);
        unsigned char getpresetpar(unsigned char npreset, unsigned int npar);
        void setpreset(unsigned char npreset);
        void changepar(int npar, unsigned char value);
        unsigned char getpar(int npar) const;
        void cleanup(void);

        static rtosc::Ports ports;
    private:
        unsigned char Pvolume;
        unsigned char PdropRate = 64;
        unsigned char PmaxDrop  = 0;
        unsigned char PfadingTime = 5;
        unsigned char PfreqOffset = 64;
        unsigned char Plfodepth = 0;

        void setvolume(unsigned char _Pvolume);

        EffectLFO lfo;
        unsigned int srate;
        CombFilterBank *filterBank;
};

}
#endif