set(zynaddsubfxgui_output_SRCS
	Recorder.C
)

if(AlsaMidiOutput)
	set(zynaddsubfxgui_output_SRCS
		${zynaddsubfxgui_output_SRCS}
		OSSaudiooutput.C)
	set(zynaddsubfxgui_output_lib ${ASOUND_LIBRARY})
endif(AlsaMidiOutput)

if(JackOutput)
	include_directories(${JACK_INCLUDE_DIR})
	set(zynaddsubfxgui_output_SRCS
		${zynaddsubfxgui_output_SRCS}
		JACKaudiooutput.C)
	set(zynaddsubfxgui_output_lib ${JACK_LIBRARIES})
endif(JackOutput)

if(PortAudioOutput)
	include_directories(${PORTAUDIO_INCLUDE_DIR})
	set(zynaddsubfxgui_output_SRCS
		${zynaddsubfxgui_output_SRCS}
		PAaudiooutput.C)
	set(zynaddsubfxgui_output_lib ${PORTAUDIO_LIBRARIES})
endif(PortAudioOutput)

add_library(zynaddsubfxgui_output STATIC
	${zynaddsubfxgui_output_SRCS} 
	)

target_link_libraries(zynaddsubfxgui_output ${zynaddsubfxgui_output_lib})
