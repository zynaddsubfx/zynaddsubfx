include ../Makefile.inc

objects=ADnote.o Envelope.o LFO.o OscilGen.o SUBnote.o Resonance.o PADnote.o


all: $(objects)

-include ../Make.deps

.PHONY : clean
clean: 
	rm -f $(objects)
	rm -f makeinclude.deps

