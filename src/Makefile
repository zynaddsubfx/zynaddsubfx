include Makefile.inc

ifneq ($(MAKECMDGOALS),debug)
  CXXFLAGS= -O6 -ggdb -Wall 
else 
  CXXFLAGS= -O0 -ggdb -Wall -Wpointer-arith -Wstrict-prototypes
endif

CXXFLAGS += -DOS_$(OS_PORT) -D$(MIDIIN)MIDIIN -D$(AUDIOOUT)AUDIOOUT -DFFTW_VERSION_$(FFTW_VERSION) -DASM_F2I_$(ASM_F2I) `fltk-config --cflags` 

export CXXFLAGS
LIBS= -lm  `fltk-config --ldflags` -lmxml -lz

ifeq ($(FFTW_VERSION),2)
LIBS += -lrfftw -lfftw
else
LIBS += -lfftw3
endif

ifeq ($(OS_PORT),LINUX) 
LIBS+= -lpthread
else
LIBS+= -lpthreadGC
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


ifeq ($(AUDIOOUT),JACK) 
CXXFLAGS += `pkg-config --cflags jack`
LIBS+= `pkg-config --libs jack`
endif

ifeq ($(AUDIOOUT),JACK_RT) 
CXXFLAGS += `pkg-config --cflags jack`
LIBS+= `pkg-config --libs jack`
endif

objects=main.o
SUBDIRS=DSP Effects Input Misc Output Params Synth Seq

.PHONY: subdirs $(SUBDIRS)

all:
	yes " " | head
	$(MAKE) -C UI $@
#	@sh -c "cd UI ; $(CXX) -MM -MG -w *.cc >> ../Make.deps ; cd .."
	@for name in $(SUBDIRS); do sh -c "cd $$name ; $(CXX) -MM -MG -w *.C >> ../Make.deps ; cd .."; done
	$(MAKE) subdirs
	$(MAKE) objs
	rm -f zynaddsubfx zynaddsubfx.exe
	rm -f Make.deps 


ifneq ($(AUDIOOUT),VST) 
	$(CXX) -o zynaddsubfx */*.o *.o $(LIBS)
else
	gcc -shared -o zynaddsubfx_vst.dll */*.o *.o $(LIBS) zynaddsubfx_gcc.def
endif

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@


objs:$(objects)

debug: all

main.o:Misc/Master.h Misc/Util.h Output/OSSaudiooutput.h\
       Input/OSSMidiIn.h Input/ALSAMidiIn.h \
       UI/MasterUI.h


.PHONY : clean
clean: 
	rm -f $(objects) zynaddsubfx zynaddsubfx_vst.dll zynaddsubfx.exe
	@for name in $(SUBDIRS); do sh -c "make -C $$name $@"; done
	rm -f Make.deps
	rm -f */*.o *.o
	$(MAKE) -C UI $@

