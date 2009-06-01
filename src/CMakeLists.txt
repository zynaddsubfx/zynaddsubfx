FIND_PACKAGE(Qt4)
INCLUDE(${QT_USE_FILE})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

#settings. These cache variables can be changed before compile
#in CMakeCache.txt
SET (FFTW_VERSION 3 CACHE STRING "Version number of FFTW")
SET (OutputModule jack CACHE STRING "Output module, either alsa, jack or portaudio")
set (WANT_FLTK ON)


if (OutputModule STREQUAL alsa)
	set(AlsaMidiOutput TRUE)
elseif(OutputModule STREQUAL jack)
	set(JackOutput TRUE)
elseif(OutputModule STREQUAL portaudio)
	set(PortAudioOutput TRUE)
else ()
	message(FATAL_ERROR "OutputModule must be either alsa, jack or portaudio")
endif()

message(STATUS "OutputModule set to ${OutputModule}")

# setting dependent stuff
if(AlsaMidiOutput)
	FIND_PACKAGE(Alsa)
	add_definitions(-DOSSAUDIOOUT)
endif()

if(JackOutput)
	FIND_PACKAGE(JACK)
	include_directories(${JACK_INCLUDE_DIR})
	add_definitions(-DJACKAUDIOOUT)
endif()

if(PortAudioOutput)
	FIND_PACKAGE(PortAudio)
	include_directories(${PORTAUDIO_INCLUDE_DIR})
	add_definitions(-DPAAUDIOOUT)
endif()

add_definitions(-DFFTW_VERSION_${FFTW_VERSION}
	 -DOS_LINUX
	 -DALSAMIDIIN
	 -DASM_F2I_YES
	 )


 if(WANT_FLTK) # Check for FLTK
    find_package(FLTK REQUIRED)
    # message(STATUS "FLTK_LIBRARIES: ${FLTK_LIBRARIES}")
    # message("FLTK_MATH_LIBRARY: ${FLTK_MATH_LIBRARY}")
    # message("FLTK_LIBRARIES: ${FLTK_LIBRARIES}")

    mark_as_advanced(FORCE FLTK_BASE_LIBRARY)
    mark_as_advanced(FORCE FLTK_CONFIG_SCRIPT)
    mark_as_advanced(FORCE FLTK_DIR)
    mark_as_advanced(FORCE FLTK_FLUID_EXECUTABLE)
    mark_as_advanced(FORCE FLTK_FORMS_LIBRARY)
    mark_as_advanced(FORCE FLTK_GL_LIBRARY)
    mark_as_advanced(FORCE FLTK_IMAGES_LIBRARY)
    mark_as_advanced(FORCE FLTK_INCLUDE_DIR)
    mark_as_advanced(FORCE FLTK_MATH_LIBRARY)

    set(FLTK_LIBRARIES
        ${FLTK_BASE_LIBRARY}    ${FLTK_MATH_LIBRARY}    ${FLTK_FORMS_LIBRARY}
        ${FLTK_IMAGES_LIBRARY}  ${FLTK_GL_LIBRARY}
    )
    add_definitions(-DWANT_GUI)
    message(STATUS "will build gui")
endif()


include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/NewUI
	${CMAKE_CURRENT_SOURCE_DIR}/NewUI)

add_subdirectory(Misc)
add_subdirectory(Input)
add_subdirectory(Output)
add_subdirectory(Controls)
#add_subdirectory(NewUI)
add_subdirectory(Synth)
add_subdirectory(Seq)
add_subdirectory(Effects)
add_subdirectory(Params)
add_subdirectory(DSP)
add_subdirectory(UI)
add_subdirectory(Samples)

set(zynaddsubfxgui_SRCS
	#main.cpp
	main.C
	)

add_executable(zynaddsubfxgui 
	${zynaddsubfxgui_SRCS}
)

target_link_libraries(zynaddsubfxgui
	zynaddsubfxgui_input
	zynaddsubfxgui_output
	zynaddsubfxgui_misc
	zynaddsubfxgui_synth
	zynaddsubfxgui_seq
	zynaddsubfxgui_effect
	zynaddsubfxgui_params
	zynaddsubfxgui_dsp
	zynaddsubfxgui_samples
	zynaddsubfxgui_controls
	${FLTK_LIBRARIES}
	zynaddsubfxguilib)

