/*
  ZynAddSubFX - a software synthesizer

  globals.h - it contains program settings and the program capabilities
              like number of parts, of effects
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/


#ifndef GLOBALS_H
#define GLOBALS_H

#if defined(__clang__)
#define REALTIME __attribute__((annotate("realtime")))
#define NONREALTIME __attribute__((annotate("nonrealtime")))
#else
#define REALTIME
#define NONREALTIME
#endif

//Forward Declarations

#if defined(HAVE_CPP_STD_COMPLEX)
#include <complex>
#else
namespace std {
    template<class T> struct complex;
}
#endif

namespace rtosc{struct Ports; struct ClonePorts; struct MergePorts; class ThreadLink;}
namespace zyn {

class  EffectMgr;
class  ADnoteParameters;
struct ADnoteGlobalParam;
class  SUBnoteParameters;
class  PADnoteParameters;
class  SynthNote;

class  Allocator;
class  AbsTime;
class  RelTime;

class  Microtonal;
class  XMLwrapper;
class  Resonance;
class  FFTwrapper;
class  EnvelopeParams;
class  LFOParams;
class  FilterParams;

struct WatchManager;
class  LFO;
class  Envelope;
class  OscilGen;

class  Controller;
class  Master;
class  Part;

class  Filter;
class  AnalogFilter;
class  MoogFilter;
class  CombFilter;
class  SVFilter;
class  FormantFilter;
class  ModFilter;

typedef float fftwf_real;
typedef std::complex<fftwf_real> fft_t;

/**
 * The number of harmonics of additive synth
 * This must be smaller than OSCIL_SIZE/2
 */
#define MAX_AD_HARMONICS 128


/**
 * The number of harmonics of substractive
 */
#define MAX_SUB_HARMONICS 64


/*
 * The maximum number of samples that are used for 1 PADsynth instrument(or item)
 */
#define PAD_MAX_SAMPLES 64


/*
 * Number of parts
 */
#define NUM_MIDI_PARTS 16

/*
 * Number of Midi channels
 */
#define NUM_MIDI_CHANNELS 16

/*
 * The number of voices of additive synth for a single note
 */
#define NUM_VOICES 8

/*
 * The polyphony (notes)
 */
#define POLYPHONY 60

/*
 * Number of system effects
 */
#define NUM_SYS_EFX 4


/*
 * Number of insertion effects
 */
#define NUM_INS_EFX 8

/*
 * Number of part's insertion effects
 */
#define NUM_PART_EFX 3

/*
 * Maximum number of the instrument on a part
 */
#define NUM_KIT_ITEMS 16


/*
 * How is applied the velocity sensing
 */
#define VELOCITY_MAX_SCALE 8.0f

/*
 * The maximum length of instrument's name
 */
#define PART_MAX_NAME_LEN 30

/*
 * The maximum we allow for an XMZ path
 *
 * Note that this is an ugly hack.  Finding a compile time path
 * max portably is painful.
 */
#define XMZ_PATH_MAX 1024

/*
 * The maximum number of bands of the equaliser
 */
#define MAX_EQ_BANDS 8
#if (MAX_EQ_BANDS >= 20)
#error "Too many EQ bands in globals.h"
#endif


/*
 * Maximum filter stages
 */
#define MAX_FILTER_STAGES 5

/*
 * Formant filter (FF) limits
 */
#define FF_MAX_VOWELS 6
#define FF_MAX_FORMANTS 12
#define FF_MAX_SEQUENCE 8

#define MAX_PRESETTYPE_SIZE 30

#define LOG_2 0.693147181f
#define PI 3.1415926536f
#define PIDIV2 1.5707963268f
#define LOG_10 2.302585093f

/*
 * For de-pop adjustment
 */
#define FADEIN_ADJUSTMENT_SCALE 20

/*
 * Envelope Limits
 */
#define MAX_ENVELOPE_POINTS 40
#define MIN_ENVELOPE_DB -400

/*
 * The threshold for the amplitude interpolation used if the amplitude
 * is changed (by LFO's or Envelope's). If the change of the amplitude
 * is below this, the amplitude is not interpolated
 */
#define AMPLITUDE_INTERPOLATION_THRESHOLD 0.0001f

/*
 * How the amplitude threshold is computed
 */
#define ABOVE_AMPLITUDE_THRESHOLD(a, b) ((2.0f * fabsf((b) - (a)) \
                                          / (fabsf((b) + (a) \
                                                  + 0.0000000001f))) > \
                                         AMPLITUDE_INTERPOLATION_THRESHOLD)

/*
 * Interpolate Amplitude
 */
#define INTERPOLATE_AMPLITUDE(a, b, x, size) ((a) \
                                              + ((b) \
                                                 - (a)) * (float)(x) \
                                              / (float) (size))


/*
 * dB
 */
#define dB2rap(dB) ((expf((dB) * LOG_10 / 20.0f)))
#define rap2dB(rap) ((20 * logf(rap) / LOG_10))

#define ZERO(data, size) {char *data_ = (char *) data; for(int i = 0; \
                                                           i < size; \
                                                           i++) \
                              data_[i] = 0; }
#define ZERO_float(data, size) {float *data_ = (float *) data; \
                                for(int i = 0; \
                                    i < size; \
                                    i++) \
                                    data_[i] = 0.0f; }

enum ONOFFTYPE {
    OFF = 0, ON = 1
};

enum MidiControllers {
    C_bankselectmsb = 0, C_pitchwheel = 1000, C_NULL = 1001,
    C_aftertouch = 1002, C_pitch = 1003,
    C_expression    = 11, C_panning = 10, C_bankselectlsb = 32,
    C_filtercutoff  = 74, C_filterq = 71, C_bandwidth = 75, C_modwheel = 1,
    C_fmamp  = 76,
    C_volume = 7, C_sustain = 64, C_allnotesoff = 123, C_allsoundsoff = 120,
    C_resetallcontrollers = 121,
    C_portamento = 65, C_resonance_center = 77, C_resonance_bandwidth = 78,

    C_dataentryhi = 0x06, C_dataentrylo = 0x26, C_nrpnhi = 99, C_nrpnlo = 98
};

enum LegatoMsg {
    LM_Norm, LM_FadeIn, LM_FadeOut, LM_CatchUp, LM_ToNorm
};

//is like i=(int)(floor(f))
#ifdef ASM_F2I_YES
#define F2I(f, i)\
    do {\
        __asm__ __volatile__\
            ("fistpl %0" : "=m" (i) : "t" (f - 0.49999999f) : "st");\
    } while (false)
#else
#define F2I(f, i)\
    do {\
        (i) = ((f > 0) ? ((int)(f)) : ((int)(f - 1.0f)));\
    } while (false)
#endif



#ifndef  O_BINARY
#define O_BINARY 0
#endif

template<class T>
class m_unique_array
{
    T* ptr = nullptr; //!< @invariant nullptr or pointer to new[]'ed memory
public:
    m_unique_array() = default;
    m_unique_array(m_unique_array&& other) : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    m_unique_array& operator=(m_unique_array&& other) {
        ptr = other.ptr;
        other.ptr = nullptr;
        return *this;
    }
    m_unique_array(const m_unique_array& other) = delete;
    ~m_unique_array() { delete[] ptr; ptr = nullptr; }
    void resize(unsigned sz) {
        delete[] ptr;
        ptr = new T[sz]; }

    operator T*() { return ptr; }
    operator const T*() const { return ptr; }
    //T& operator[](unsigned idx) { return ptr[idx]; }
    //const T& operator[](unsigned idx) const { return ptr[idx]; }
};

//temporary include for synth->{samplerate/buffersize} members
struct SYNTH_T {

    SYNTH_T(void)
        :samplerate(44100), buffersize(256), oscilsize(1024)
    {
        alias(false);
    }

    SYNTH_T(const SYNTH_T& ) = delete;
    SYNTH_T(SYNTH_T&& ) = default;
    SYNTH_T& operator=(const SYNTH_T& ) = delete;
    SYNTH_T& operator=(SYNTH_T&& ) = default;

    /** the buffer to add noise in order to avoid denormalisation */
    m_unique_array<float> denormalkillbuf;

    /**Sampling rate*/
    unsigned int samplerate;

    /**
     * The size of a sound buffer (or the granularity)
     * All internal transfer of sound data use buffer of this size.
     * All parameters are constant during this period of time, except
     * some parameters(like amplitudes) which are linearly interpolated.
     * If you increase this you'll encounter big latencies, but if you
     * decrease this the CPU requirements gets high.
     */
    int buffersize;

    /**
     * The size of ADnote Oscillator
     * Decrease this => poor quality
     * Increase this => CPU requirements gets high (only at start of the note)
     */
    int oscilsize;

    //Alias for above terms
    float samplerate_f;
    float halfsamplerate_f;
    float buffersize_f;
    int   bufferbytes;
    float oscilsize_f;

    float dt(void) const
    {
        return buffersize_f / samplerate_f;
    }
    void alias(bool randomize=true);
    static float numRandom(void); //defined in Util.cpp for now
};

class smooth_float {
private:
    bool init;
    float curr_value;
    float next_value;
public:
    smooth_float() {
        init = false;
        next_value = curr_value = 0.0f;
    };
    smooth_float(const float value) {
        init = true;
        next_value = curr_value = value;
    };
    operator float() {
        const float delta = (next_value - curr_value) / 32.0f;
        curr_value += delta;
        return (curr_value);
    };
    void operator =(const float value) {
      if (init) {
          next_value = value;
      } else {
          next_value = curr_value = value;
          init = true;
      }
    };
    bool isSet() const {
        return (init);
    };
};

}
#endif
