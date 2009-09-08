include Makefile.inc

ifneq ($(MAKECMDGOALS),debug)
  CXXFLAGS= -O2 -Wall -g
else 
  CXXFLAGS= -O2 -Wall -Wpointer-arith
endif

CXXFLAGS += -DOS_$(OS_PORT) -D$(MIDIIN)MIDIIN -DFFTW_VERSION_$(FFTW_VERSION) -DASM_F2I_$(ASM_F2I) -ggdb

ifeq ($(DISABLE_GUI),YES)
    CXXFLAGS += -DDISABLE_GUI
else
    CXXFLAGS += -DFLTK_GUI `fltk-config --cflags` 
endif

ifeq ($(AUDIOOUT),OSS_AND_JACK)
    CXXFLAGS += -DOSSAUDIOOUT -DJACKAUDIOOUT 
else
    CXXFLAGS += -D$(AUDIOOUT)AUDIOOUT 
endif

export CXXFLAGS

LIBS= -lm  -lmxml -lz
ifneq ($(DISABLE_GUI),YES)
    LIBS+=`fltk-config --ldflags` 
endif

ifeq ($(FFTW_VERSION),2)
LIBS += -lrfftw -lfftw
else
LIBS += -lfftw3
endif

ifeq ($(OS_PORT),LINUX) 
LIBS+= -lpthread
else
#dynamic linking
#LIBS+= -lpthreadGC
#static linking
LIBS+= /usr/lib/libpthreadGC1.a
endif

ifeq ($(MIDIIN),ALSA) 
LIBS+= -lasound
endif

ifeq ($(AUDIOOUT),PA) 
LIBS+= -lportaudio
endif

ifeq ($(OS_PORT),WINDOWS) 
LIBS+= -lwinmm
endif


ifeq ($(AUDIOOUT),OSS_AND_JACK) 
CXXFLAGS += `pkg-config --cflags jack`
LIBS+= `pkg-config --libs jack`
endif

ifeq ($(AUDIOOUT),JACK) 
CXXFLAGS += `pkg-config --cflags jack`
LIBS+= `pkg-config --libs jack`
endif

ifeq ($(AUDIOOUT),JACK_RT) 
CXXFLAGS += `pkg-config --cflags jack`
LIBS+= `pkg-config --libs jack`
endif

ifeq ($(LINUX_USE_LASH),YES)
CXXFLAGS += `pkg-config --cflags lash-1.0` -DUSE_LASH
LIBS+= `pkg-config --libs lash-1.0`
endif


objects=main.o
SUBDIRS=Controls DSP Effects Input Misc Output Params Samples Synth Seq

.PHONY: subdirs $(SUBDIRS)

all:
#	yes " " | head

	$(MAKE) -C UI $@
#	@sh -c "cd UI ; $(CXX) -MM -MG -w *.cc >> ../Make.deps ; cd .."
	@for name in $(SUBDIRS); do sh -c "cd $$name ; $(CXX) -MM -MG -w *.cpp >> ../Make.deps ; cd .."; done
	$(MAKE) subdirs
	$(MAKE) objs
	rm -f zynaddsubfx zynaddsubfx.exe
	rm -f Make.deps 


ifeq ($(AUDIOOUT),DSSI) 
	$(CXX) -shared  -o zynaddsubfx.so */*.o *.o $(LIBS) 
else
ifeq ($(AUDIOOUT),VST) 
	$(CXX) -shared -o zynaddsubfx_vst.dll */*.o *.o ../../vstsdk2/source/common/AudioEffect.cpp ../../vstsdk2/source/common/audioeffectx.cpp  $(LIBS) zynaddsubfx_gcc.def
else


ifeq ($(OS_PORT),LINUX) 
	$(CXX) -o zynaddsubfx */*.o *.o $(LIBS)
else
	$(CXX) -o zynaddsubfx.exe */*.o *.o $(LIBS)
endif 
endif
endif

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@


objs:$(objects)

debug: all

main.o:Misc/Master.h Misc/Util.h Output/OSSaudiooutput.h\
       Input/OSSMidiIn.h Input/ALSAMidiIn.h
       

.PHONY : clean
clean: 
	rm -f $(objects) zynaddsubfx zynaddsubfx_vst.dll zynaddsubfx.exe zynaddsubfx.so
	@for name in $(SUBDIRS); do sh -c "make -C $$name $@"; done
	rm -f Make.deps
	rm -f */*.o *.o
	rm -f ./*/*~ ./*~
	$(MAKE) -C UI $@

