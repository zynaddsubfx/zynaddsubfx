include ../Makefile.inc

objects=NULLMidiIn.o MidiIn.o

ifeq ($(MIDIIN),ALSA) 
objects+=ALSAMidiIn.o 
endif

ifeq ($(MIDIIN),OSS) 
objects+=OSSMidiIn.o 
endif

ifeq ($(MIDIIN),WIN) 
objects+=WINMidiIn.o 
endif


all: $(objects)

-include ../Make.deps

.PHONY : clean
clean: 
	rm -f $(objects)
	rm -f makeinclude.deps

