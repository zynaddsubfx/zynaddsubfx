set(zynaddsubfxgui_seq_SRCS
	MIDIEvents.C
	MIDIFile.C
	Sequencer.C
)

add_library(zynaddsubfxgui_seq STATIC
	${zynaddsubfxgui_seq_SRCS} 
	)

target_link_libraries(zynaddsubfxgui_seq ${ASOUND_LIBRARY})
