set(zynaddsubfxgui_samples_SRCS
	AuSample.C
	FqSample.C
	Sample.C
	#StereoSample.C
)

add_library(zynaddsubfxgui_samples STATIC
	${zynaddsubfxgui_samples_SRCS} 
	)

target_link_libraries(zynaddsubfxgui_samples 
	zynaddsubfxgui_samples
	)
