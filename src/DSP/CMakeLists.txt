set(zynaddsubfxgui_dsp_SRCS
	AnalogFilter.C
	FFTwrapper.C
	Filter.C
	FormantFilter.C
	SVFilter.C
)

add_library(zynaddsubfxgui_dsp STATIC
	${zynaddsubfxgui_dsp_SRCS} 
	)

target_link_libraries(zynaddsubfxgui_dsp ${ASOUND_LIBRARY})
