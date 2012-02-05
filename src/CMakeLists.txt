#checking include/library paths
message(STATUS "Checking Include Path" $ENV{CMAKE_INCLUDE_PATH} ${CMAKE_INCLUDE_PATH})
message(STATUS "Checking Library Path" $ENV{CMAKE_LIBRARY_PATH} ${CMAKE_LIBRARY_PATH})

#Dependency check
find_package(zlib REQUIRED)
find_package(fftw REQUIRED)
find_package(MXML REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Threads   REQUIRED)
find_package(OSS)
find_package(Alsa)
find_package(JACK)
find_package(PortAudio)
set(FLTK_SKIP_OPENGL true)
find_package(FLTK)
find_package(OpenGL) #for FLTK
find_package(CxxTest)
# lash
pkg_search_module(LASH lash-1.0)
mark_as_advanced(LASH_LIBRARIES)
pkg_search_module(DSSI dssi>=0.9.0)
mark_as_advanced(DSSI_LIBRARIES)

######### Settings ###########
# NOTE: These cache variables should normally not be changed in this
# file, but either in in CMakeCache.txt before compile, or by passing
# parameters directly into cmake using the -D flag.
SET (DefaultOutput oss CACHE STRING
    "Default Output module: [null, alsa, oss, jack, portaudio]")
SET (GuiModule fltk CACHE STRING "GUI module, either fltk, qt or off")
SET (CompileTests ${CXXTEST_FOUND} CACHE BOOL "whether tests should be compiled in or not")
SET (AlsaEnable ${ALSA_FOUND} CACHE BOOL
    "Enable support for Advanced Linux Sound Architecture")
SET (JackEnable ${JACK_FOUND} CACHE BOOL
    "Enable support for JACK Audio Connection toolKit")
SET (OssEnable ${OSS_FOUND} CACHE BOOL
    "Enable support for Open Sound System")
SET (PaEnable ${PORTAUDIO_FOUND} CACHE BOOL
    "Enable support for Port Audio System")
SET (LashEnable ${LASH_FOUND} CACHE BOOL
    "Enable LASH Audio Session Handler")
SET (DssiEnable ${DSSI_FOUND} CACHE BOOL
    "Enable DSSI Plugin compilation")


# Now, handle the incoming settings and set define flags/variables based
# on this

# Add version information
add_definitions(-DVERSION="${VERSION}")

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

#Build Flags
option (BuildForAMD_X86_64 "Build for AMD x86_64 system" OFF)
option (BuildForCore2_X86_64 "Build for Intel Core2 x86_64 system" OFF)
option (BuildForDebug "Include gdb debugging support" OFF)

set(CMAKE_BUILD_TYPE "Release")

set (BuildOptions_x86_64AMD
    "-O3 -march=athlon64 -m64 -Wall -ffast-math -fno-finite-math-only -fomit-frame-pointer"
  CACHE STRING "X86_64 compiler options"
)

set (BuildOptions_X86_64Core2
    "-O3 -march=core2 -m64 -Wall -ffast-math -fno-finite-math-only -fomit-frame-pointer"
  CACHE STRING "X86_64 compiler options"
)

set (BuildOptionsBasic
    "-O3 -msse -msse2 -mfpmath=sse -ffast-math -fomit-frame-pointer"
    CACHE STRING "basic X86 complier options"
)

set (BuildOptionsDebug
    "-O0 -g3 -ggdb -Wall -Wpointer-arith" CACHE STRING "Debug build flags")

########### Settings dependant code ###########
# From here on, the setting variables have  been prepared so concentrate
# on the actual compiling.

if(AlsaEnable)
	list(APPEND AUDIO_LIBRARIES ${ASOUND_LIBRARY})
	list(APPEND AUDIO_LIBRARY_DIRS ${ASOUND_LIBRARY_DIRS})
	add_definitions(-DALSA=1)
endif(AlsaEnable)

if(JackEnable)
	list(APPEND AUDIO_LIBRARIES ${JACK_LIBRARIES})
	list(APPEND AUDIO_LIBRARY_DIRS ${JACK_LIBRARY_DIRS})
	add_definitions(-DJACK=1)
endif(JackEnable)

if(OssEnable)
	add_definitions(-DOSS=1)
endif(OssEnable)

if(PaEnable)
	include_directories(${PORTAUDIO_INCLUDE_DIR})
	add_definitions(-DPORTAUDIO=1)
	list(APPEND AUDIO_LIBRARIES ${PORTAUDIO_LIBRARIES})
	list(APPEND AUDIO_LIBRARY_DIRS ${PORTAUDIO_LIBRARY_DIRS})
endif()

if (CompileTests)
	ENABLE_TESTING()
endif()

if(LashEnable)
	include_directories(${LASH_INCLUDE_DIRS})
	add_definitions(-DLASH=1)
	list(APPEND AUDIO_LIBRARIES ${LASH_LIBRARIES})
	list(APPEND AUDIO_LIBRARY_DIRS ${LASH_LIBRARY_DIRS})
	message(STATUS "Compiling with lash")
endif()

# other include directories
include_directories(${ZLIB_INCLUDE_DIRS} ${MXML_INCLUDE_DIRS})

add_definitions(
	 -DASM_F2I_YES
	 -g #TODO #todo put in a better location
	 -Wall
	 -Wextra
	 )

if (BuildForDebug)
	set (CMAKE_BUILD_TYPE "Debug")
	set (CMAKE_CXX_FLAGS_DEBUG ${BuildOptionsDebug})
	message (STATUS "Building for ${CMAKE_BUILD_TYPE}, flags: ${CMAKE_CXX_FLAGS_DEBUG}")
else (BuildForDebug)
	set (CMAKE_BUILD_TYPE "Release")
	if (BuildForAMD_X86_64)
		set (CMAKE_CXX_FLAGS_RELEASE ${BuildOptions_x86_64AMD})
	else (BuildForAMD_X86_64)
		if (BuildForCore2_X86_64)
			set (CMAKE_CXX_FLAGS_RELEASE ${BuildOptions_X86_64Core2})
		else (BuildForCore2_X86_64)
			set (CMAKE_CXX_FLAGS_RELEASE ${BuildOptionsBasic})
		endif (BuildForCore2_X86_64)
	endif (BuildForAMD_X86_64)
	message (STATUS "Building for ${CMAKE_BUILD_TYPE}, flags: ${CMAKE_CXX_FLAGS_RELEASE}")
endif (BuildForDebug)

if (DssiEnable)
	add_definitions(-fPIC)
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

link_directories(${AUDIO_LIBRARY_DIRS} ${ZLIB_LIBRARY_DIRS} ${FFTW_LIBRARY_DIRS} ${MXML_LIBRARY_DIRS} ${FLTK_LIBRARY_DIRS})

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	)



set(NONGUI_LIBRARIES
	zynaddsubfx_misc
	zynaddsubfx_synth
	zynaddsubfx_effect
	zynaddsubfx_params
	zynaddsubfx_dsp
	zynaddsubfx_nio
	)

add_subdirectory(Misc)
add_subdirectory(Synth)
add_subdirectory(Effects)
add_subdirectory(Params)
add_subdirectory(DSP)
if(CompileTests)
	add_subdirectory(Tests)
endif(CompileTests)
add_subdirectory(Nio)

set(zynaddsubfx_SRCS
	main.cpp
	)

message(STATUS "using link directories: ${AUDIO_LIBRARY_DIRS} ${ZLIB_LIBRARY_DIRS} ${FFTW_LIBRARY_DIRS} ${MXML_LIBRARY_DIRS} ${FLTK_LIBRARY_DIRS}")


add_executable(zynaddsubfx
	${zynaddsubfx_SRCS}
)

target_link_libraries(zynaddsubfx
	${NONGUI_LIBRARIES}
	${GUI_LIBRARIES}
	${ZLIB_LIBRARIES}
	${FFTW_LIBRARIES}
	${MXML_LIBRARIES}
	${NIO_LIBRARIES}
	${AUDIO_LIBRARIES}
	${OS_LIBRARIES}
	pthread
	)

if (DssiEnable)
	add_library(zynaddsubfx_dssi SHARED
		Output/DSSIaudiooutput.cpp
		)

	target_link_libraries(zynaddsubfx_dssi
		${NONGUI_LIBRARIES}
		${ZLIB_LIBRARIES}
		${FFTW_LIBRARY}
		${MXML_LIBRARY}
		${NIO_LIBRARIES}
		${AUDIO_LIBRARIES}
		${OS_LIBRARIES}
		)
	if (${CMAKE_SIZEOF_VOID_P} EQUAL "8")
		install(TARGETS zynaddsubfx_dssi LIBRARY DESTINATION lib64/dssi/)
	else ()
		install(TARGETS zynaddsubfx_dssi LIBRARY DESTINATION lib/dssi/)
	endif ()
endif()

message(STATUS "Link libraries: ${ZLIB_LIBRARY} ${FFTW_LIBRARY} ${MXML_LIBRARY} ${AUDIO_LIBRARIES} ${OS_LIBRARIES}")
install(TARGETS zynaddsubfx
	RUNTIME DESTINATION bin
	)

include(CTest)
