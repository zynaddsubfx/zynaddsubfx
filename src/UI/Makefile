include ../Makefile.inc

%.cc  : %.fl
	fluid -c $<

objects=WidgetPDial.o PresetsUI.o EnvelopeUI.o LFOUI.o FilterUI.o VirKeyboard.o ConfigUI.o\
	SUBnoteUI.o ResonanceUI.o OscilGenUI.o ADnoteUI.o PADnoteUI.o EffUI.o BankUI.o \
	PartUI.o MicrotonalUI.o SeqUI.o MasterUI.o 
ifeq ($(DISABLE_GUI),YES)
    objects=
endif

all: $(objects)

WidgetPDial.o: WidgetPDial.fl WidgetPDial.cc WidgetPDial.h
PresetsUI.o: PresetsUI.fl PresetsUI.cc PresetsUI.h
EnvelopeUI.o: EnvelopeUI.fl EnvelopeUI.cc EnvelopeUI.h
LFOUI.o: LFOUI.fl LFOUI.cc LFOUI.h
FilterUI.o: FilterUI.fl FilterUI.cc FilterUI.h

ResonanceUI.o:ResonanceUI.fl ResonanceUI.cc ResonanceUI.h
OscilGenUI.o:OscilGenUI.fl OscilGenUI.cc OscilGenUI.h
ADnoteUI.o:ADnoteUI.fl ADnoteUI.cc ADnoteUI.h
SUBnoteUI.o:SUBnoteUI.fl SUBnoteUI.cc SUBnoteUI.h
PADnoteUI.o:PADnoteUI.fl PADnoteUI.cc PADnoteUI.h

EffUI.o: EffUI.fl EffUI.cc EffUI.h
BankUI.o: BankUI.fl BankUI.cc BankUI.h
PartUI.o: PartUI.fl PartUI.cc PartUI.h

VirKeyboard.o:  VirKeyboard.fl VirKeyboard.cc VirKeyboard.h
ConfigUI.o: ConfigUI.fl ConfigUI.cc ConfigUI.h
MicrotonalUI.o: MicrotonalUI.fl MicrotonalUI.cc MicrotonalUI.h
SeqUI.o: SeqUI.fl SeqUI.cc SeqUI.h
MasterUI.o:  MasterUI.fl MasterUI.cc MasterUI.h

.PHONY : clean
clean: 
	rm -f $(objects)
	rm -f makeinclude.deps
	rm -f *.h *.cc
