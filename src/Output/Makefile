include ../Makefile.inc

objects=Recorder.o 

ifeq ($(AUDIOOUT),JACK)
objects+=JACKaudiooutput.o
endif

ifeq ($(AUDIOOUT),PA)
objects+=PAaudiooutput.o
endif

ifeq ($(AUDIOOUT),OSS)
objects+=OSSaudiooutput.o
endif

ifeq ($(AUDIOOUT),VST)
objects+=VSTaudiooutput.o
endif


all: $(objects)

-include ../Make.deps

.PHONY : clean
clean: 
	rm -f $(objects)
	rm -f makeinclude.deps

