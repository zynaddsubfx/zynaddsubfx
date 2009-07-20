#Dependency check
find_package(zlib REQUIRED)
find_package(fftw REQUIRED)
find_package(MXML REQUIRED)
#find_package(pthread REQUIRED)
find_package(Alsa)
find_package(JACK)
find_package(PortAudio)
find_package(FLTK)

######### Settings ###########
# NOTE: These cache variables should normallly not be changed in this
# file, but either in in CMakeCache.txt before compile, or by passing
# parameters directly into cmake using the -D flag.
SET (FFTW_VERSION 3 CACHE STRING "Version number of FFTW")
SET (OutputModule alsa CACHE STRING "Output module, either alsa, jack or portaudio")
SET (GuiModule fltk CACHE STRING "GUI module, either fltk, qt or off")

# Now, handle the incoming settings and set define flags/variables based
# on this

if (GuiModule STREQUAL qt AND QT_FOUND)
	set (QtGui TRUE)
elseif(GuiModule STREQUAL fltk AND FLTK_FOUND)
	set (FltkGui TRUE)
elseif(GuiModule STREQUAL off)
        add_definitions(-DDISABLE_GUI)
else  ()
        set (GuiModule off CACHE STRING "GUI module, either fltk, qt or off")
        add_definitions(-DDISABLE_GUI)
	message(STATUS "GUI module defaulting to off")
endif()

if (OutputModule STREQUAL alsa AND ALSA_FOUND)
	set(AlsaMidiOutput TRUE)
elseif(OutputModule STREQUAL jack AND JACK_FOUND)
	set(JackOutput TRUE)
elseif(OutputModule STREQUAL portaudio AND PortAudio_FOUND)
	set(PortAudioOutput TRUE)
else ()
	message(FATAL_ERROR "OutputModule must be either alsa, jack or portaudio")
endif()

########### Settings dependant code ###########
# From here on, the setting variables have  been prepared so concentrate
# on the actual compiling.

if(AlsaMidiOutput)
	add_definitions(-DOSSAUDIOOUT)
        set(AUDIO_LIBRARIES ${AUDIO_LIBRARIES} ${ASOUND_LIBRARY})
endif()

if(JackOutput)
	include_directories(${JACK_INCLUDE_DIR})
	add_definitions(-DJACKAUDIOOUT)
        set(AUDIO_LIBRARIES ${AUDIO_LIBRARIES} ${JACK_LIBRARIES})
endif()

if(PortAudioOutput)
	include_directories(${PORTAUDIO_INCLUDE_DIR})
	add_definitions(-DPAAUDIOOUT)
        set(AUDIO_LIBRARIES ${AUDIO_LIBRARIES} ${PORTAUDIO_LIBRARIES})
endif()

add_definitions(-DFFTW_VERSION_${FFTW_VERSION}
	 -DOS_LINUX
	 -DALSAMIDIIN
	 -DASM_F2I_YES
	 )


if(FltkGui)
    #message(STATUS "FLTK_LIBRARIES: ${FLTK_LIBRARIES}")
    #message(STATUS "FLTK_MATH_LIBRARY: ${FLTK_MATH_LIBRARY}")

    #UGLY WORKAROUND
    find_program (MYFLTK_CONFIG fltk-config)
    if (MYFLTK_CONFIG)
        execute_process (COMMAND ${MYFLTK_CONFIG} --ldflags OUTPUT_VARIABLE MYFLTK_LDFLAGS)
        string(STRIP ${MYFLTK_LDFLAGS} MYFLTK_LIBRARIES)
    endif()
    mark_as_advanced(FORCE FLTK_BASE_LIBRARY)
    mark_as_advanced(FORCE FLTK_CONFIG_SCRIPT)
    mark_as_advanced(FORCE FLTK_DIR)
    mark_as_advanced(FORCE FLTK_FLUID_EXECUTABLE)
    mark_as_advanced(FORCE FLTK_FORMS_LIBRARY)
    mark_as_advanced(FORCE FLTK_GL_LIBRARY)
    mark_as_advanced(FORCE FLTK_IMAGES_LIBRARY)
    mark_as_advanced(FORCE FLTK_INCLUDE_DIR)
    mark_as_advanced(FORCE FLTK_MATH_LIBRARY)


    set(GUI_LIBRARIES ${FLTK_LIBRARIES} ${MYFLTK_LIBRARIES} zynaddsubfx_gui)
    add_definitions(-DFLTK_GUI)

    message(STATUS "Will build fltk gui")

	include_directories(
			"${CMAKE_CURRENT_SOURCE_DIR}/UI"
			"${CMAKE_CURRENT_BINARY_DIR}/UI"
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
        ${zlib_LIBRARIES}
        ${fftw_LIBRARIES}
        ${MXML_LIBRARIES}
        ${AUDIO_LIBRARIES}
	)

