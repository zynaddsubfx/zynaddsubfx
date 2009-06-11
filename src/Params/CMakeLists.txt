set(zynaddsubfxgui_params_SRCS
	ADnoteParameters.C
	Controller.C
	EnvelopeParams.C
	FilterParams.C
	LFOParams.C
	PADnoteParameters.C
	Presets.C
	PresetsStore.C
	SUBnoteParameters.C

)

add_library(zynaddsubfxgui_params STATIC
	${zynaddsubfxgui_params_SRCS} 
	)

target_link_libraries(zynaddsubfxgui_params ${ASOUND_LIBRARY})
