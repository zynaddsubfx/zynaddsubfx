include ../Makefile.inc

objects=ADnoteParameters.o EnvelopeParams.o FilterParams.o \
        LFOParams.o SUBnoteParameters.o PADnoteParameters.o Controller.o Presets.o PresetsArray.o PresetsStore.o


all: $(objects)

-include ../Make.deps

.PHONY : clean
clean: 
	rm -f $(objects)
	rm -f makeinclude.deps

