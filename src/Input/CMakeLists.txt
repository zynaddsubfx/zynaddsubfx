SET (AlsaMidiInput On CACHE BOOL "Include ALSA Midi input")
set(zynaddsubfxgui_input_SRCS
	MidiIn.C 
	NULLMidiIn.C 
	#OSSMidiIn.C
	#WINMidiIn.C
)
FIND_PACKAGE(Alsa)

if(AlsaMidiInput)
	set(zynaddsubfxgui_input_SRCS
		${zynaddsubfxgui_input_SRCS}
		ALSAMidiIn.C
		)
	message("Alsa midi input enabled")
endif(AlsaMidiInput)

add_library(zynaddsubfxgui_input STATIC
	${zynaddsubfxgui_input_SRCS} 
	)

target_link_libraries(zynaddsubfxgui_input ${QT_LIBRARIES}  ${ASOUND_LIBRARY})
