/*
  ZynAddSubFX - a software synthesizer

  Nio.h - IO Interface
  Copyright (C) 2016 Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#ifndef NIO_H
#define NIO_H
#include <string>
#include <set>

namespace zyn {

class WavFile;
class Master;
struct SYNTH_T;
class oss_devs_t;

/**Interface to Nio Subsystem
 *
 * Should be only externally included header */
namespace Nio
{
    void init(const SYNTH_T &synth, const oss_devs_t &oss_devs, Master *master);
    bool start(void);
    void stop(void);

    void setDefaultSource(std::string name);
    void setDefaultSink(std::string name);

    bool setSource(std::string name);
    bool setSink(std::string name);

    void setPostfix(std::string post);
    std::string getPostfix(void);

    std::set<std::string> getSources(void);
    std::set<std::string> getSinks(void);

    std::string getSource(void);
    std::string getSink(void);

    //Get the preferred sample rate from jack (if running)
    void preferredSampleRate(unsigned &rate);

    //Complete Master Swaps to ONLY BE CALLED FROM RT CONTEXT
    void masterSwap(Master *master);

    //Wave writing
    void waveNew(class WavFile *wave);
    void waveStart(void);
    void waveStop(void);
    void waveEnd(void);
    
    void setAudioCompressor(bool isEnabled);
    bool getAudioCompressor(void);

    extern bool autoConnect;
    extern bool pidInClientName;
    extern std::string defaultSource;
    extern std::string defaultSink;
};

}

#endif
