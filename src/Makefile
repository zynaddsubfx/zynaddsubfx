include Makefile.inc

ifneq ($(MAKECMDGOALS),debug)
  CXXFLAGS= -O6 
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

ifeq ($(MIDIIN),WIN) 
LIBS+= -lwinmm
endif


ifeq ($(AUDIOOUT),PA) 
LIBS+= -lportaudio
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
subdirs=DSP Effects Input Misc Output Params Synth Seq

all:
	yes " " | head
	$(MAKE) -C UI $@
	rm -f Make.deps 
	@for name in $(subdirs); do sh -c "cd $$name ; $(CXX) -MM -MG -w *.C >> ../Make.deps ; cd .."; done
	@for name in $(subdirs); do sh -c "make -C $$name $@"; done
	$(MAKE) objs
	rm -f zynaddsubfx zynaddsubfx.exe


ifneq ($(AUDIOOUT),VST) 
	$(CXX) -o zynaddsubfx */*.o *.o $(LIBS)
else
	gcc -mdll -o temp1.tmp -Wl,--base-file,temp2.tmp */*.o *.o $(LIBS)
	dlltool --dllname zynaddsubfx_vst.dll --def zynaddsubfx_gcc.def --base-file temp2.tmp --output-exp temp3.tmp
	gcc -mdll -o zynaddsubfx_vst.dll */*.o *.o $(LIBS) -Wl,temp3.tmp
	rm temp1.tmp temp2.tmp temp3.tmp
endif

objs:$(objects)

debug: all

main.o:Misc/Master.h Misc/Util.h Output/OSSaudiooutput.h\
       Input/OSSMidiIn.h Input/ALSAMidiIn.h \
       UI/MasterUI.h


.PHONY : clean
clean: 
	rm -f $(objects) makeinclude.deps zynaddsubfx zynaddsubfx_vst.dll zynaddsubfx.exe
	@for name in $(subdirs); do sh -c "make -C $$name $@"; done
	rm -f Make.deps
	rm -f */*.o *.o
	$(MAKE) -C UI $@

