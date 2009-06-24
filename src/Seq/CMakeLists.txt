set(zynaddsubfx_seq_SRCS
	MIDIEvents.C
	MIDIFile.C
	Sequencer.C
)

add_library(zynaddsubfx_seq STATIC
	${zynaddsubfx_seq_SRCS} 
	)

target_link_libraries(zynaddsubfx_seq ${ASOUND_LIBRARY})
