find_package(MXML)
find_package(fftw3)

include_directories(${MXML_INCLUDE_DIR})

set(zynaddsubfx_misc_SRCS
	Bank.C
	Config.C
	Dump.C
	Master.C
	Microtonal.C
	Part.C
	Util.C
	XMLwrapper.C
)

add_library(zynaddsubfx_misc STATIC
	${zynaddsubfx_misc_SRCS} 
	)

target_link_libraries(zynaddsubfx_misc 
	zynaddsubfx_output
	${MXML_LIBRARIES}
	${fftw3_LIBRARIES}
	)
