set(zynaddsubfx_output_SRCS
	Recorder.C
	WAVaudiooutput.C
)

if(AlsaMidiOutput)
	set(zynaddsubfx_output_SRCS
		${zynaddsubfx_output_SRCS}
		OSSaudiooutput.C)
	set(zynaddsubfx_output_lib ${ASOUND_LIBRARY})
endif(AlsaMidiOutput)

if(JackOutput)
	include_directories(${JACK_INCLUDE_DIR})
	set(zynaddsubfx_output_SRCS
		${zynaddsubfx_output_SRCS}
		JACKaudiooutput.C)
	set(zynaddsubfx_output_lib ${JACK_LIBRARIES})
endif(JackOutput)

if(PortAudioOutput)
	include_directories(${PORTAUDIO_INCLUDE_DIR})
	set(zynaddsubfx_output_SRCS
		${zynaddsubfx_output_SRCS}
		PAaudiooutput.C)
	set(zynaddsubfx_output_lib ${PORTAUDIO_LIBRARIES})
endif(PortAudioOutput)

add_library(zynaddsubfx_output STATIC
	${zynaddsubfx_output_SRCS} 
	)

target_link_libraries(zynaddsubfx_output ${zynaddsubfx_output_lib})
