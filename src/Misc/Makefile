include ../Makefile.inc

objects=Bank.o Master.o Microtonal.o Part.o Util.o Config.o Dump.o XMLwrapper.o


all: $(objects)

-include ../Make.deps

.PHONY : clean
clean: 
	rm -f $(objects)
	rm -f makeinclude.deps

