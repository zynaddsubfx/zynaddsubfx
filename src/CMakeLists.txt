#Dependency check
find_package(zlib REQUIRED)
find_package(fftw REQUIRED)
find_package(MXML REQUIRED)
#find_package(pthread REQUIRED)
find_package(Alsa)
find_package(JACK)
find_package(PortAudio)
set(FLTK_SKIP_OPENGL true)
find_package(FLTK)
find_package(OpenGL) #for FLTK

######### Settings ###########
# NOTE: These cache variables should normally not be changed in this
# file, but either in in CMakeCache.txt before compile, or by passing
# parameters directly into cmake using the -D flag.
SET (FFTW_VERSION 3 CACHE STRING "Version number of FFTW")
SET (OutputModule alsa CACHE STRING "Output module, either alsa, nio, jack or portaudio")
SET (GuiModule fltk CACHE STRING "GUI module, either fltk, qt or off")
SET (CompileTests OFF CACHE BOOL "whether tests should be compiled in or not")

#change this to have defaults set by results of find_package
SET (AlsaEnable OFF CACHE BOOL
    "Enable support for Advanced Linux Sound Architecture")
SET (JackEnable OFF CACHE BOOL
    "Enable support for JACK Audio Connection toolKit")
SET (OssEnable OFF CACHE BOOL
    "Enable support for Open Sound System")

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
elseif(OutputModule STREQUAL nio AND ALSA_FOUND)
        set(NewIO TRUE)
else ()
	message(FATAL_ERROR "OutputModule must be either alsa, nio, jack or portaudio")
endif()

########### Settings dependant code ###########
# From here on, the setting variables have  been prepared so concentrate
# on the actual compiling.

if(AlsaMidiOutput)
	add_definitions(-DOSSAUDIOOUT)
        set(AUDIO_LIBRARIES ${AUDIO_LIBRARIES} ${ASOUND_LIBRARY})
endif()

if(NewIO)
	add_definitions(-DNEW_IO)
        set(AUDIO_LIBRARIES ${AUDIO_LIBRARIES} ${ASOUND_LIBRARY})
endif()

if (CompileTests)
	ENABLE_TESTING()
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
         -g #TODO #todo put in a better location
         -Wall
         -Wextra
	 )

if(FLTK_FOUND)
    mark_as_advanced(FORCE FLTK_BASE_LIBRARY)
    mark_as_advanced(FORCE FLTK_CONFIG_SCRIPT)
    mark_as_advanced(FORCE FLTK_DIR)
    mark_as_advanced(FORCE FLTK_FLUID_EXECUTABLE)
    mark_as_advanced(FORCE FLTK_FORMS_LIBRARY)
    mark_as_advanced(FORCE FLTK_GL_LIBRARY)
    mark_as_advanced(FORCE FLTK_IMAGES_LIBRARY)
    mark_as_advanced(FORCE FLTK_INCLUDE_DIR)
    mark_as_advanced(FORCE FLTK_MATH_LIBRARY)
endif(FLTK_FOUND)

if(FltkGui)
    #UGLY WORKAROUND
    find_program (MYFLTK_CONFIG fltk-config)
    if (MYFLTK_CONFIG)
        execute_process (COMMAND ${MYFLTK_CONFIG} --ldflags OUTPUT_VARIABLE MYFLTK_LDFLAGS)
        string(STRIP ${MYFLTK_LDFLAGS} MYFLTK_LIBRARIES)
    endif()

    message(STATUS ${MYFLTK_LDFLAGS})


    set(GUI_LIBRARIES  ${MYFLTK_LIBRARIES} ${FLTK_LIBRARIES} ${OPENGL_LIBRARIES} zynaddsubfx_gui)

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


#for tests looking for files stored in the source dir
add_definitions(-DSOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
#macro for tests
macro(unit_test NAME CXX_FILE FILES)
    #	if (CompileTests)
		set(PATH_FILES "")
		foreach(part ${FILES})
			set(PATH_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${part}" ${PATH_FILES})
		endforeach(part ${FILES})
		set(CXX_FILE_REAL "${CMAKE_CURRENT_SOURCE_DIR}/${CXX_FILE}")
		add_custom_command(
			OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.cxx"
			COMMAND cxxtestgen.py --error-printer -o "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.cxx" ${CXX_FILE_REAL}
			DEPENDS "${FILE}"
			)
		set(CXXTEST_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.cxx")
		add_executable("${NAME}" "${CXXTEST_OUTPUT}" ${PATH_FILES})
		target_link_libraries("${NAME}" ${CXXTEST_LINK_LIBS}
				${NONGUI_LIBRARIES}
				${GUI_LIBRARIES}
				${zlib_LIBRARIES}
				${fftw_LIBRARIES}
				${MXML_LIBRARIES}
				)
		add_test("${NAME}" "${EXECUTABLE_OUTPUT_PATH}/${NAME}")
                #endif()
endmacro(unit_test)

set(NONGUI_LIBRARIES
	zynaddsubfx_input
	zynaddsubfx_misc
	zynaddsubfx_synth
	zynaddsubfx_seq
	zynaddsubfx_effect
	zynaddsubfx_params
        zynaddsubfx_output
	zynaddsubfx_dsp
	zynaddsubfx_samples
	zynaddsubfx_controls
        zynaddsubfx_nio
    )

set(CXXTEST_LINK_LIBS ${NONGUI_LIBRARIES})
set(MIDIINPUT_LIBRARIES "")

add_subdirectory(Misc)
add_subdirectory(Input)
add_subdirectory(Controls)
add_subdirectory(Synth)
add_subdirectory(Output)
add_subdirectory(Seq)
add_subdirectory(Effects)
add_subdirectory(Params)
add_subdirectory(DSP)
add_subdirectory(Samples)
add_subdirectory(Tests)
add_subdirectory(Nio)

set(zynaddsubfx_SRCS
	main.cpp
	)

add_executable(zynaddsubfx
	${zynaddsubfx_SRCS}
)

target_link_libraries(zynaddsubfx
    ${NONGUI_LIBRARIES}
	${GUI_LIBRARIES}
        ${zlib_LIBRARIES}
        ${fftw_LIBRARIES}
        ${MXML_LIBRARIES}
        ${NIO_LIBRARIES}
        ${AUDIO_LIBRARIES}
        ${MIDIINPUT_LIBRARIES}
	)

