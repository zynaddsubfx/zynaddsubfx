include ../Makefile.inc

objects=FFTwrapper.o AnalogFilter.o FormantFilter.o SVFilter.o Filter.o Unison.o


all: $(objects)

-include ../Make.deps

.PHONY : clean
clean: 
	rm -f $(objects)
	rm -f makeinclude.deps

