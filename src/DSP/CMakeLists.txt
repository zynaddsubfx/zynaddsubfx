set(zynaddsubfx_dsp_SRCS
	AnalogFilter.C
	FFTwrapper.C
	Filter.C
	FormantFilter.C
	SVFilter.C
)

add_library(zynaddsubfx_dsp STATIC
	${zynaddsubfx_dsp_SRCS} 
	)

target_link_libraries(zynaddsubfx_dsp ${ASOUND_LIBRARY})
