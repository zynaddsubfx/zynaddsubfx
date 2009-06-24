set(zynaddsubfx_synth_SRCS
	ADnote.C
	Envelope.C
	LFO.C
	OscilGen.C
	PADnote.C
	Resonance.C
	SUBnote.C
)

add_library(zynaddsubfx_synth STATIC
	${zynaddsubfx_synth_SRCS} 
	)

target_link_libraries(zynaddsubfx_synth ${ASOUND_LIBRARY})
