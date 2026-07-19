#include <cmath>
#include <rtosc/ports.h>
#include <rtosc/port-sugar.h>
#include "../Misc/Allocator.h"
#include "PitchDrop.h"
#include "CombFilterBank.h"

namespace zyn {

#define rObject PitchDrop
#define rBegin [](const char *msg, rtosc::RtData &d) {
#define rEnd }

const float drop_freeverb_freqs[8] = {1116.0f, 1188.0f, 1277.0f, 1356.0f, 1422.0f, 1491.0f, 1557.0f, 1617.0f};

rtosc::Ports PitchDrop::ports = {
    {"preset::i", rProp(parameter)
                  rOptions(Freeverb)
                  rProp(alias)
                  rDefault(0)
                  rDoc("Preset"), 0,
                  rBegin;
                  rObject *o = (rObject*)d.obj;
                  if(rtosc_narguments(msg))
                      o->setpreset(rtosc_argument(msg, 0).i);
                  else
                      d.reply(d.loc, "i", o->Ppreset);
                  rEnd},
    rEffParVol(rDefault(90)),
    rEffParPan(rDefault(64)),
    rEffPar(PdropRate, 2, rShort("drop"), rDefault(64), "Pitch Drop Rate"),
    rEffPar(PmaxDrop, 3, rShort("max"), rDefault(0), "Max Drop"),
    rEffPar(PfadingTime, 4, rShort("fade"), rDefault(5), "Fading Time"),
    rEffPar(PfreqOffset, 5, rShort("offset"), rDefault(64), "Pitch Offset"),
    rEffPar(lfo.Pfreq, 6, rShort("freq"), rDefault(32), "LFO frequency"),
    rEffPar(lfo.Prandomness, 7, rShort("rnd"), rDefault(0), "LFO randomness"),
    rEffParOpt(lfo.PLFOtype, 8, rShort("type"), rDefault(sine),
            rOptions(sine, tri), "LFO shape"),
    rEffPar(lfo.Pstereo, 9, rShort("stereo"), rDefault(64),
            "LFO stereo phase"),
    rEffPar(Plfodepth, 10, rShort("mod"), rDefault(0), "LFO modulation depth"),
};

#undef rBegin
#undef rEnd
#undef rObject

PitchDrop::PitchDrop(EffectParams pars)
    :Effect(pars),
      Pvolume(90),
      lfo(pars.srate, pars.bufsize),
      srate(pars.srate)
{
    filterBank = memory.alloc<CombFilterBank>(&memory, pars.srate, pars.bufsize, 0.938f);

    // Setup 8 freeverb-style comb filters
    const unsigned int nstrings = 8;
    for(unsigned int i = 0; i < nstrings; i++)
        filterBank->delays[i] = ((float)samplerate) * drop_freeverb_freqs[i] / 44100.0f;

    const unsigned int mem_size_new = (int)ceilf((filterBank->delays[0] * 32.0f * 1.03f + buffersize + 2) / 16) * 16;
    filterBank->setStrings(nstrings, mem_size_new);

    setpreset(Ppreset);
    cleanup();
}

PitchDrop::~PitchDrop()
{
    memory.dealloc(filterBank);
}

void PitchDrop::cleanup(void)
{
}

void PitchDrop::out(const Stereo<float *> &smp)
{
    float inputvol = powf(2.0f, 0.0f) / 2.0f;

    // Apply LFO modulation to dropRate
    Stereo<float> lfoVal(0.0f, 0.0f);
    lfo.effectlfoout(&lfoVal.l, &lfoVal.r);
    float baseDrop = (float)(PdropRate - 64) / (-256.0f * float(srate));
    float lfoAmp = lfoVal.l * (float)Plfodepth / 127.0f * 64.0f / (-256.0f * float(srate));
    filterBank->dropRate = baseDrop + lfoAmp;

    for(int i = 0; i < buffersize; ++i)
        efxoutl[i] = (smp.l[i] * pangainL + smp.r[i] * pangainR) * inputvol;

    filterBank->filterout(efxoutl);

    memcpy(efxoutr, efxoutl, bufferbytes);

    float level = dB2rap(60.0f * Pvolume / 127.0f - 40.0f);
    for(int i = 0; i < buffersize; ++i) {
        float lout = efxoutl[i];
        float rout = efxoutr[i];
        float l    = lout * (1.0f - lrcross) + rout * lrcross;
        float r    = rout * (1.0f - lrcross) + lout * lrcross;
        lout = l;
        rout = r;

        efxoutl[i] = lout * 2.0f * level;
        efxoutr[i] = rout * 2.0f * level;
    }
}

void PitchDrop::setvolume(unsigned char _Pvolume)
{
    Pvolume = _Pvolume;

    if(insertion == 0) {
        outvolume = powf(0.01f, (1.0f - Pvolume / 127.0f)) * 4.0f;
        volume    = 1.0f;
    }
    else
        volume = outvolume = Pvolume / 127.0f;
    if(Pvolume == 0)
        cleanup();
}

unsigned char PitchDrop::getpresetpar(unsigned char npreset, unsigned int npar)
{
    static const unsigned char presets[1][11] = {
        //Vol Pan drop max fade offset freq rnd type stereo mod
        {90, 64, 64, 0, 5, 64, 32, 0, 0, 64, 0},
    };
    if(npreset < 1 && npar < 11) {
        if(npar == 0 && insertion == 0)
            return (2 * presets[npreset][npar]) / 3;
        return presets[npreset][npar];
    }
    return 0;
}

void PitchDrop::setpreset(unsigned char npreset)
{
    if(npreset >= 1)
        npreset = 0;
    for(int n = 0; n != 128; n++)
        changepar(n, getpresetpar(npreset, n));
    Ppreset = npreset;
}

void PitchDrop::changepar(int npar, unsigned char value)
{
    switch(npar) {
        case 0:
            setvolume(value);
            break;
        case 1:
            setpanning(value);
            break;
        case 2:
            PdropRate = value;
            filterBank->dropRate = (float)(PdropRate-64)/(-256.0f * float(srate));
            break;
        case 3:
            PmaxDrop = value;
            filterBank->maxDrop = (float)(PmaxDrop+1)/32.0f;
            break;
        case 4:
            PfadingTime = value;
            filterBank->fadingTime = (float)value/127.0f;
            break;
        case 5:
            PfreqOffset = value;
            filterBank->pitchOffset = (float)(PfreqOffset-64)/-64.0f;
            break;
        case 6:
            lfo.Pfreq = value;
            lfo.updateparams();
            break;
        case 7:
            lfo.Prandomness = value;
            lfo.updateparams();
            break;
        case 8:
            lfo.PLFOtype = value;
            lfo.updateparams();
            break;
        case 9:
            lfo.Pstereo = value;
            lfo.updateparams();
            break;
        case 10:
            Plfodepth = value;
            break;
        default:
            break;
    }
}

unsigned char PitchDrop::getpar(int npar) const
{
    switch(npar) {
        case 0:  return Pvolume;
        case 1:  return Ppanning;
        case 2:  return PdropRate;
        case 3:  return PmaxDrop;
        case 4:  return PfadingTime;
        case 5:  return PfreqOffset;
        case 6:  return lfo.Pfreq;
        case 7:  return lfo.Prandomness;
        case 8:  return lfo.PLFOtype;
        case 9:  return lfo.Pstereo;
        case 10: return Plfodepth;
        default: return 0;
    }
}

}
