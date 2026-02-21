/*
  ZynAddSubFX - a software synthesizer

  Portamento.h - Portamento calculation and management
  Copyright (C) 2021 Mark McCurry
  Author: Mark McCurry

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/
#ifndef PORTAMENTO_H
#define PORTAMENTO_H
#include "../globals.h"
#include "../Params/Controller.h"
#include <functional>

namespace zyn {

// Realtime struct governing portamento. Read by synth engines,
// created and managed by parts.
class Portamento {
    public:
        /**
         * Create a portamento.
         * Sets the active member if the portemento is activated.
         *
         * @param ctl The Controller which contains user patch parameters
         * @param synth The SYNTH_T from which to get sample rate and bufsize
         * @param is_running_note True if at least one note is playing
         * @param oldfreq_log2 Pitch of previous note
         * @param oldportamentofreq_log2 Starting pitch of the portamento
         * @param newfreq_log2 Ending pitch of the portamento
         */
        Portamento(const Controller &ctl,
                   const SYNTH_T &synth,
                   bool is_running_note,
                   float oldfreq_log2,
                   float oldportamentofreq_log2,
                   float newfreq_log2);
        /**
         * Initialize an already existing portamento.
         * Sets the active member if the portemento is activated.
         *
         * @param ctl The Controller which contains user patch parameters
         * @param synth The SYNTH_T from which to get sample rate and bufsize
         * @param is_running_note True if at least one note is playing
         * @param oldfreq_log2 Pitch of previous note
         * @param oldportamentofreq_log2 Starting pitch of the portamento
         * @param newfreq_log2 Ending pitch of the portamento
         */
        void init(const Controller &ctl,
                  const SYNTH_T &synth,
                  bool is_running_note,
                  float oldfreq_log2,
                  float oldportamentofreq_log2,
                  float newfreq_log2);
        /**Update portamento's freqrap to next value based upon dx*/
        void update(void);
        /**if the portamento is in use*/
        bool active;
        /**this value is used to compute the actual portamento
         *
         * This is the logarithmic power of two frequency
         * adjustment of the newer frequency to fit the profile of
         * the portamento.
         * This will be linear with respect to x.*/
        float freqdelta_log2;

    private:
        /**x is from 0.0f (start portamento) to 1.0f (finished portamento)*/
        float x;
        /**dx is the increment to x when update is called*/
        float dx;
        /** this is used for computing freqdelta_log2 value from x*/
        float origfreqdelta_log2;
};

class PortamentoRealtime {
    public:
        /**
         * Create a portamento realtime structure.
         *
         * @param handle handle to be used by cleanup function
         * @param memory Allocator used
         * @param cleanup Callback called from destructor
         * @param portamento Portamento object to be contained
         */
        PortamentoRealtime(void *handle,
                           Allocator &memory,
                           std::function<void(PortamentoRealtime *)> cleanup,
                           const Portamento &portamento);

        ~PortamentoRealtime();

        /**handle to be used by cleanup function in lieu of lambda capture*/
        void *handle;
        /**Allocator used to allocate memory*/
        Allocator &memory;
        /**Cleanup callback called by destructor*/
        std::function<void(PortamentoRealtime *)> cleanup;
        /**The actual portamento object*/
        Portamento portamento;
};

}

#endif /* PORTAMENTO_H */
