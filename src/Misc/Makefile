include ../Makefile.inc

objects=Bank.o Buffer.o Master.o Microtonal.o Part.o Util.o Config.o Dump.o


all: $(objects)

-include ../Make.deps

.PHONY : clean
clean: 
	rm -f $(objects)
	rm -f makeinclude.deps

