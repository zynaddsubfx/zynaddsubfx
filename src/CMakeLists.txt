######### Settings ###########
# NOTE: These cache variables should normallly not be changed in this
# file, but either in in CMakeCache.txt before compile, or by passing
# parameters directly into cmake using the -D flag.
SET (FFTW_VERSION 3 CACHE STRING "Version number of FFTW")
SET (OutputModule jack CACHE STRING "Output module, either alsa, jack or portaudio")
SET (GuiModule fltk CACHE STRING "GUI module, either fltk, qt or off")

# Now, handle the incoming settings and set define flags/variables based
# on this

if (GuiModule STREQUAL qt)
	set (QtGui TRUE)
elseif(GuiModule STREQUAL fltk)
	set (FltkGui TRUE)
elseif(GuiModule STREQUAL off)
    add_definitions(-DDISABLE_GUI)
else  ()
	message(FATAL_ERROR "GUI module must be either fltk, qt or off")
endif()

if (OutputModule STREQUAL alsa)
	set(AlsaMidiOutput TRUE)
elseif(OutputModule STREQUAL jack)
	set(JackOutput TRUE)
elseif(OutputModule STREQUAL portaudio)
	set(PortAudioOutput TRUE)
else ()
	message(FATAL_ERROR "OutputModule must be either alsa, jack or portaudio")
endif()

########### Settings dependant code ###########
# From here on, the setting variables have  been prepared so concentrate
# on the actual compiling.

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

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


if(FltkGui) # Check for FLTK
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


    set(GUI_LIBRARIES
        ${FLTK_BASE_LIBRARY}    ${FLTK_MATH_LIBRARY}    ${FLTK_FORMS_LIBRARY}
        ${FLTK_IMAGES_LIBRARY}  ${FLTK_GL_LIBRARY}
  zynaddsubfx_gui
  )
    add_definitions(-DFLTK_GUI)

    message(STATUS "Will build fltk gui")

	include_directories(
			${CMAKE_CURRENT_SOURCE_DIR}/UI
			${CMAKE_CURRENT_BINARY_DIR}/UI
			)

	add_subdirectory(UI)
endif()


########### General section ##############
# Following this should be only general compilation code, and no mention
# of module-specific variables

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	)

add_subdirectory(Misc)
add_subdirectory(Input)
add_subdirectory(Output)
add_subdirectory(Controls)
add_subdirectory(Synth)
add_subdirectory(Seq)
add_subdirectory(Effects)
add_subdirectory(Params)
add_subdirectory(DSP)
add_subdirectory(Samples)

set(zynaddsubfx_SRCS
	main.cpp
	)

add_executable(zynaddsubfx 
	${zynaddsubfx_SRCS}
)

target_link_libraries(zynaddsubfx
	zynaddsubfx_input
	zynaddsubfx_output
	zynaddsubfx_misc
	zynaddsubfx_synth
	zynaddsubfx_seq
	zynaddsubfx_effect
	zynaddsubfx_params
	zynaddsubfx_dsp
	zynaddsubfx_samples
	zynaddsubfx_controls
	${GUI_LIBRARIES}
	)

