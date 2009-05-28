set(zynaddsubfxgui_synth_SRCS
	ADnote.C
	Envelope.C
	LFO.C
	OscilGen.C
	PADnote.C
	Resonance.C
	SUBnote.C
)

add_library(zynaddsubfxgui_synth STATIC
	${zynaddsubfxgui_synth_SRCS} 
	)

target_link_libraries(zynaddsubfxgui_synth ${ASOUND_LIBRARY})
