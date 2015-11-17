/*
  ZynAddSubFX - a software synthesizer

  VSTaudiooutput.h - Audio output for VST
  Copyright (C) 2002 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/
#ifndef VST_AUDIO_OUTPUT_H
#define VST_AUDIO_OUTPUT_H

#include <thread>

#include "../globals.h"
#include "../Misc/Config.h"
#include "../Misc/MiddleWare.h"

#include <dssi.h>
#include <ladspa.h>
#include <vector>

class DSSIaudiooutput
{
    public:
        //
        // Static stubs for LADSPA member functions
        //
        static void stub_connectPort(LADSPA_Handle instance,
                                     unsigned long port,
                                     LADSPA_Data *data);
        static void stub_activate(LADSPA_Handle instance);
        static void stub_run(LADSPA_Handle instance, unsigned long sample_count);
        static void stub_deactivate(LADSPA_Handle Instance);
        static void stub_cleanup(LADSPA_Handle instance);

        //
        // Static stubs for DSSI member functions
        //
        static const DSSI_Program_Descriptor *stub_getProgram(
            LADSPA_Handle instance,
            unsigned long Index);
        static void stub_selectProgram(LADSPA_Handle instance,
                                       unsigned long bank,
                                       unsigned long program);
        static int stub_getMidiControllerForPort(LADSPA_Handle instance,
                                                 unsigned long port);
        static void stub_runSynth(LADSPA_Handle instance,
                                  unsigned long sample_count,
                                  snd_seq_event_t *events,
                                  unsigned long event_count);

        /*
         * LADSPA member functions
         */
        static LADSPA_Handle instantiate(const LADSPA_Descriptor *descriptor,
                                         unsigned long s_rate);
        void connectPort(unsigned long port, LADSPA_Data *data);
        void activate();
        void run(unsigned long sample_count);
        void deactivate();
        void cleanup();
        static const LADSPA_Descriptor *getLadspaDescriptor(unsigned long index);

        /*
         * DSSI member functions
         */
        const DSSI_Program_Descriptor *getProgram(unsigned long Index);
        void selectProgram(unsigned long bank, unsigned long program);
        int getMidiControllerForPort(unsigned long port);
        void runSynth(unsigned long sample_count,
                      snd_seq_event_t *events,
                      unsigned long event_count);
        static const DSSI_Descriptor *getDssiDescriptor(unsigned long index);

        struct ProgramDescriptor {
            unsigned long bank;
            unsigned long program;
            std::string   name;
        };

        /*
         * DSSI Controls mapping
         */
        const static int MAX_DSSI_CONTROLS = 12;

        /**
         * DSSIControl represent one instance of DSSI control used to describe accepted values to the DSSI host
         * and to forward DSSI host value change to ZynAddSubFx controller
         */
        class DSSIControl {
        public:
            const MidiControllers controller_code; /// controler code, as accepted by the Controller class
            const char *name; /// human readable name of this control

            /** hint about usable range of value for this control, defaulting to 0-128, initially at 64 */
            const LADSPA_PortRangeHint port_range_hint = {
                    LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE, 0, 128};


            float *data; /// pointer to the value for this controller which is updated by the DSSI host

            /**
             * Ctr for a DSSIControl using the default hint (0-128 starting at 64)
             * @param controller_code the controller code
             * @param name the human readable code name
             */
            DSSIControl(MidiControllers controller_code, const char *name) : controller_code(controller_code), name(name) {}

            /**
             * Ctr for a DSSIControl
             * @param controller_code the controller code
             * @param name the human readable code name
             * @param port_range_hint the accepted range of values
             */
            DSSIControl(MidiControllers controller_code, const char *name, LADSPA_PortRangeHint port_range_hint) :
                    controller_code(controller_code), name(name), port_range_hint(port_range_hint) {}

            /**
             * update the current control to the Master in charge of dispatching them to the parts, effects, ...
             * @param master the controller master in charge of further dispatch
             */
            void forward_control(Master *master);

            /**
             * scale the incoming value refereced by data in the hinted range to one expected by the Master dispatcher.
             * Boolean are toggled to 0 or 127, ...
             */
            int get_scaled_data() {
                if (LADSPA_IS_HINT_TOGGLED(port_range_hint.HintDescriptor)) {
                    return *data <= 0 ? 0 : 127;
                } else if (port_range_hint.UpperBound < 127) {
                    // when not using 127 or 128 as upper bound, scale the input using the port range hint to 0 .. 128
                    return 128 * ( *data - port_range_hint.LowerBound ) / ( port_range_hint.UpperBound - port_range_hint.LowerBound );
                } else {
                    return *data;
                }
            }

        };

    private:

        DSSIaudiooutput(unsigned long sampleRate);
        ~DSSIaudiooutput();
        static DSSI_Descriptor *initDssiDescriptor();
        static DSSIaudiooutput *getInstance(LADSPA_Handle instance);
        void initBanks();
        bool mapNextBank();

        LADSPA_Data *outl;
        LADSPA_Data *outr;
        long    sampleRate;
        MiddleWare *middleware;
        std::thread *loadThread;
        static DSSI_Descriptor *dssiDescriptor;
        static std::string      bankDirNames[];
        static
        std::vector<ProgramDescriptor> programMap;

        /**
         * Flag controlling the list of bank directories
         */
        bool banksInited;

        static long bankNoToMap;
        Config config;
};

#endif
