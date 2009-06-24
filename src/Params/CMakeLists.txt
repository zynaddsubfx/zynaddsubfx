set(zynaddsubfx_params_SRCS
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

add_library(zynaddsubfx_params STATIC
	${zynaddsubfx_params_SRCS} 
	)

target_link_libraries(zynaddsubfx_params ${ASOUND_LIBRARY})
