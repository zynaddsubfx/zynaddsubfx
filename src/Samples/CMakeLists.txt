set(zynaddsubfx_samples_SRCS
	AuSample.C
	FqSample.C
	Sample.C
	#StereoSample.C
)

add_library(zynaddsubfx_samples STATIC
	${zynaddsubfx_samples_SRCS} 
	)

target_link_libraries(zynaddsubfx_samples 
	zynaddsubfx_samples
	)
