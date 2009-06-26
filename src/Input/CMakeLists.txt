SET (AlsaMidiInput On CACHE BOOL "Include ALSA Midi input")
set(zynaddsubfx_input_SRCS
	MidiIn.cpp 
	NULLMidiIn.cpp 
	#OSSMidiIn.cpp
	#WINMidiIn.cpp
)
FIND_PACKAGE(Alsa)

if(AlsaMidiInput)
	set(zynaddsubfx_input_SRCS
		${zynaddsubfx_input_SRCS}
		ALSAMidiIn.cpp
		)
	message("Alsa midi input enabled")
endif(AlsaMidiInput)

add_library(zynaddsubfx_input STATIC
	${zynaddsubfx_input_SRCS} 
	)

target_link_libraries(zynaddsubfx_input ${QT_LIBRARIES}  ${ASOUND_LIBRARY})
