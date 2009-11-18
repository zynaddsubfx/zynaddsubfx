set(zynaddsubfx_nio_SRCS
	OssEngine.cpp
	NulEngine.cpp
	WavEngine.cpp
	AlsaEngine.cpp
	AudioOut.cpp
	OutMgr.cpp
)

add_library(zynaddsubfx_nio STATIC
	${zynaddsubfx_nio_SRCS} 
	)

target_link_libraries(zynaddsubfx_nio)
