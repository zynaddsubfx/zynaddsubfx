find_package(MXML)
find_package(fftw3)

include_directories(${MXML_INCLUDE_DIR})

set(zynaddsubfxgui_misc_SRCS
	Bank.C
	Config.C
	Dump.C
	Master.C
	Microtonal.C
	Part.C
	Util.C
	XMLwrapper.C
)

add_library(zynaddsubfxgui_misc STATIC
	${zynaddsubfxgui_misc_SRCS} 
	)

target_link_libraries(zynaddsubfxgui_misc 
	zynaddsubfxgui_output
	${MXML_LIBRARIES}
	${fftw3_LIBRARIES}
	)
