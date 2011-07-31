#checking include/library paths
message(STATUS "Checking Include Path" $ENV{CMAKE_INCLUDE_PATH} ${CMAKE_INCLUDE_PATH})
message(STATUS "Checking Library Path" $ENV{CMAKE_LIBRARY_PATH} ${CMAKE_LIBRARY_PATH})

#Dependency check
find_package(zlib REQUIRED)
find_package(fftw REQUIRED)
find_package(MXML REQUIRED)
find_package(PkgConfig REQUIRED)
#find_package(pthread REQUIRED)
#find_package(OSS)
find_package(Alsa)
find_package(JACK)
find_package(PortAudio)
set(FLTK_SKIP_OPENGL true)
find_package(FLTK)
find_package(OpenGL) #for FLTK
# lash
pkg_search_module(LASH lash-1.0)
mark_as_advanced(LASH_LIBRARIES)

######### Settings ###########
# NOTE: These cache variables should normally not be changed in this
# file, but either in in CMakeCache.txt before compile, or by passing
# parameters directly into cmake using the -D flag.
SET (DefaultOutput oss CACHE STRING
    "Default Output module: [null, alsa, oss, jack, portaudio]")
SET (GuiModule fltk CACHE STRING "GUI module, either fltk, qt or off")
SET (CompileTests OFF CACHE BOOL "whether tests should be compiled in or not")
SET (AlsaEnable ${ALSA_FOUND} CACHE BOOL
    "Enable support for Advanced Linux Sound Architecture")
SET (JackEnable ${JACK_FOUND} CACHE BOOL
    "Enable support for JACK Audio Connection toolKit")
SET (OssEnable ${ALSA_FOUND} CACHE BOOL #TODO perhaps check for /dev/dsp
    "Enable support for Open Sound System")
SET (PaEnable ${PORTAUDIO_FOUND} CACHE BOOL
    "Enable support for Port Audio System")
SET (LashEnable ${LASH_FOUND} CACHE BOOL
    "Enable LASH Audio Session Handler")


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

if(NOT PKG_CONFIG_FOUND)
    message(FATAL_ERROR "pkg-config not found")
endif(NOT PKG_CONFIG_FOUND)


########### Settings dependant code ###########
# From here on, the setting variables have  been prepared so concentrate
# on the actual compiling.

if(AlsaEnable)
        list(APPEND AUDIO_LIBRARIES ${ASOUND_LIBRARY})
        add_definitions(-DALSA=1)
endif(AlsaEnable)

if(JackEnable)
        list(APPEND AUDIO_LIBRARIES ${JACK_LIBRARIES})
        add_definitions(-DJACK=1)
endif(JackEnable)

if(OssEnable)
	add_definitions(-DOSSAUDIOOUT)
        list(APPEND AUDIO_LIBRARIES ${ASOUND_LIBRARY})
        add_definitions(-DOSS=1)
endif(OssEnable)

if(PaEnable)
	include_directories(${PORTAUDIO_INCLUDE_DIR})
	add_definitions(-DPORTAUDIO=1)
        list(APPEND AUDIO_LIBRARIES ${PORTAUDIO_LIBRARIES})
endif()

if (CompileTests)
	ENABLE_TESTING()
endif()

if(LashEnable)
	include_directories(${LASH_INCLUDE_DIRS})
	add_definitions(-DLASH=1)
	set(AUDIO_LIBRARIES ${AUDIO_LIBRARIES} ${LASH_LIBRARIES})
	message(STATUS "Compiling with lash")
endif()

add_definitions(
	 -DASM_F2I_YES
         -g #TODO #todo put in a better location
         -Wall
         -Wextra
	 )

#set os flag
if(CYGWIN)
	add_definitions(-DOS_CYGWIN=1)
	set(OS_LIBRARIES "-Wl,--enable-auto-import")
elseif(WINDOWS)
	add_definitions(-DOS_WINDOWS=1)
	set(OS_LIBRARIES "")
elseif(UNIX)
	add_definitions(-DOS_LINUX=1)
	set(OS_LIBRARIES "-lpthread")
else()
        message(STATUS "Error: building on unsupported OS")
endif()


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


    set(GUI_LIBRARIES ${FLTK_LIBRARIES} ${MYFLTK_LIBRARIES} ${OPENGL_LIBRARIES} zynaddsubfx_gui)

    add_definitions(-DFLTK_GUI)

    message(STATUS "Will build fltk gui")

	include_directories(
			${FLTK_INCLUDE_DIR}
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
    if (CompileTests)
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
            ${OS_LIBRARIES}
            )
        add_test("${NAME}" "${EXECUTABLE_OUTPUT_PATH}/${NAME}")
    endif()
endmacro(unit_test)

set(NONGUI_LIBRARIES
	zynaddsubfx_misc
	zynaddsubfx_synth
	zynaddsubfx_seq
	zynaddsubfx_effect
	zynaddsubfx_params
	zynaddsubfx_dsp
	zynaddsubfx_samples
        zynaddsubfx_nio
    )

set(CXXTEST_LINK_LIBS ${NONGUI_LIBRARIES})

add_subdirectory(Misc)
add_subdirectory(Synth)
add_subdirectory(Seq)
add_subdirectory(Effects)
add_subdirectory(Params)
add_subdirectory(DSP)
add_subdirectory(Samples)
if(CompileTests)
    add_subdirectory(Tests)
endif(CompileTests)
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
        ${OS_LIBRARIES}
	)

install(TARGETS zynaddsubfx
	RUNTIME DESTINATION bin
	)
