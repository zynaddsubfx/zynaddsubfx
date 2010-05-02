CXX=g++

#You can set the on what OS is compiling (Linux/Windows)
OS_PORT=LINUX
#OS_PORT=WINDOWS

#The version of the FFTW which is used (2 or 3)
#FFTW_VERSION=2
FFTW_VERSION=3

#Assembler FLOAT to INT conversions
ASM_F2I=YES
#ASM_F2I=NO

#Graphic user interface disable option (ZynAddSubFX will run only in text-mode)
#DISABLE_GUI=YES
DISABLE_GUI=NO

# L I N U X   C O N F I G U R A T I O N
#Next line sets the midi input. It can be "ALSA", "OSS" or "NONE".
LINUX_MIDIIN=ALSA
#LINUX_MIDIIN=OSS
#LINUX_MIDIIN=NONE

#Next lines sets the audio output (OSS/JACK/PA)
#You may use only one at the time
#If you use "OSS_AND_JACK",,at runtime, zynaddsubfx will run by the default with jack support and 
#it will try OSS if JACK fails. At runtime you can set the OSS by default by command-line
#parameters (run 'zynaddsubfx --help' for help) 

#LINUX_AUDIOOUT=OSS_AND_JACK
LINUX_AUDIOOUT=OSS
#LINUX_AUDIOOUT=NONE
#LINUX_AUDIOOUT=JACK
#LINUX_AUDIOOUT=JACK_RT     JACK_RT support is broken
#for PortAudio (PA)
#LINUX_AUDIOOUT=PA


#Next line sets if the synth is compiled for DSSI plugin (as .so file)
#If this setting is "YES", MIDI in and AUDIOOUT are set automatically to DSSI
LINUX_DSSI=NO
#LINUX_DSSI=YES

#Next line sets if the LASH session handler will be used
#LINUX_USE_LASH=YES
LINUX_USE_LASH=NO

# W I N D O W S   C O N F I G U R A T I O N

#Next line sets the midi input
#WINDOWS_MIDIIN=NONE
WINDOWS_MIDIIN=WIN

#Next line sets the audio output
#WINDOWS_AUDIOOUT=NONE
WINDOWS_AUDIOOUT=PA

#Next line sets if the synth is compiled for VST (as .dll file)
#If this setting is "YES", MIDI in and AUDIOOUT are set automatically to VST
WINDOWS_VST=NO
#WINDOWS_VST=YES

#configuration end 

ifeq ($(OS_PORT),LINUX)
 MIDIIN=$(LINUX_MIDIIN)
 AUDIOOUT=$(LINUX_AUDIOOUT)
 WINDOWS_VST=NO
    ifeq ($(LINUX_DSSI),YES)
     DISABLE_GUI=YES
     MIDIIN=DSSI
     AUDIOOUT=DSSI
    endif
else
 MIDIIN=$(WINDOWS_MIDIIN)
 AUDIOOUT=$(WINDOWS_AUDIOOUT)
 LINUX_DSSI=NO
    ifeq ($(WINDOWS_VST),YES)
     MIDIIN=VST
     AUDIOOUT=VST
    endif
endif

