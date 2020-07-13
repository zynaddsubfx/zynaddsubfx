#checking include/library paths
message(STATUS "Checking Include Path" $ENV{CMAKE_INCLUDE_PATH} ${CMAKE_INCLUDE_PATH})
message(STATUS "Checking Library Path" $ENV{CMAKE_LIBRARY_PATH} ${CMAKE_LIBRARY_PATH})

#Dependency check
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)
if(NOT (${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    find_package(PkgConfig)
endif()
find_package(ZLIB REQUIRED)
find_package(X11)
find_package(Threads   REQUIRED)
find_package(OSS)
find_package(Alsa)
find_package(Sndio)
find_package(FLTK)
find_package(OpenGL) #for FLTK
find_package(CxxTest)
if(CXXTEST_FOUND)
    set(CXXTEST_USE_PYTHON TRUE)
endif()
# lash
if(PKG_CONFIG_FOUND AND NOT (${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
    message("Looking For pkg config modules")
    pkg_check_modules(JACK jack)
    pkg_check_modules(PORTAUDIO portaudio-2.0>=19)
    set(FLTK_SKIP_OPENGL true)
    pkg_check_modules(NTK ntk)
    pkg_check_modules(NTK_IMAGES ntk_images)

    pkg_check_modules(FFTW REQUIRED fftw3)
    pkg_check_modules(MXML REQUIRED mxml)

    pkg_search_module(LASH lash-1.0)
    mark_as_advanced(LASH_LIBRARIES)
    pkg_search_module(DSSI dssi>=0.9.0)
    mark_as_advanced(DSSI_LIBRARIES)
    pkg_search_module(LIBLO liblo>=0.26 REQUIRED)
    mark_as_advanced(LIBLO_LIBRARIES)
else()
    find_package(FFTW REQUIRED)
    find_package(MXML REQUIRED)
    find_package(LIBLO REQUIRED)
    find_package(PORTAUDIO)
endif()

CHECK_FUNCTION_EXISTS(sched_setscheduler HAVE_SCHEDULER)
set(CMAKE_REQUIRED_FLAGS "-std=c++11")
CHECK_CXX_SOURCE_COMPILES(
    "#include <future>
    #if ATOMIC_INT_LOCK_FREE<2
    #error unreliable atomics
    #endif
    int main(){return 0;}" HAVE_ASYNC)

check_cxx_compiler_flag("-msse2" SUPPORT_SSE)
check_cxx_compiler_flag("-mfpu=neon -Werror" SUPPORT_NEON)
check_include_file_cxx(complex HAVE_CPP_STD_COMPLEX)

find_library(HAVE_LIBRT rt)

set(CMAKE_REQUIRED_FLAGS "")


execute_process(COMMAND echo fistpl 0
                COMMAND as -
                ERROR_VARIABLE AVOID_ASM)

######### Settings ###########
# NOTE: These cache variables should normally not be changed in this
# file, but either in in CMakeCache.txt before compile, or by passing
# parameters directly into cmake using the -D flag.
if(NTK_FOUND)
  SET (GuiModule ntk  CACHE STRING "GUI module, either fltk, ntk, zest, or off")
elseif(FLTK_FOUND)
  SET (GuiModule fltk CACHE STRING "GUI module, either fltk, ntk, zest, or off")
else()
  SET (GuiModule off  CACHE STRING "GUI module, either fltk, ntk, zest, or off")
endif()
SET (CompileTests ${CXXTEST_FOUND} CACHE BOOL "whether tests should be compiled in or not")
SET (AlsaEnable ${ALSA_FOUND} CACHE BOOL
    "Enable support for Advanced Linux Sound Architecture")
SET (JackEnable ${JACK_FOUND} CACHE BOOL
    "Enable support for JACK Audio Connection toolKit")
SET (OssEnable ${OSS_FOUND} CACHE BOOL
    "Enable support for Open Sound System")
SET (PaEnable ${PORTAUDIO_FOUND} CACHE BOOL
    "Enable support for Port Audio System")
SET (SndioEnable ${SNDIO_FOUND} CACHE BOOL
    "Enable support for Sndio System")
SET (LashEnable ${LASH_FOUND} CACHE BOOL
    "Enable LASH Audio Session Handler")
SET (DssiEnable ${DSSI_FOUND} CACHE BOOL
    "Enable DSSI Plugin compilation")
SET (NoNeonPlease False CACHE BOOL
    "Workaround For Broken Neon Detection")
SET (PluginLibDir "lib" CACHE STRING
    "Install directory for plugin libraries PREFIX/PLUGIN_LIB_DIR/{lv2,vst}")
SET (DemoMode FALSE CACHE BOOL "Enable 10 minute silence")
SET (ZynFusionDir "" CACHE STRING "Developers only: zest binary's dir; useful if fusion is not system-installed.")
mark_as_advanced(FORCE ZynFusionDir)

# Now, handle the incoming settings and set define flags/variables based
# on this

# Add version information
add_definitions(-DVERSION="${VERSION}")
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  add_definitions(-static)
endif()

if(HAVE_SCHEDULER AND (NOT "Darwin" STREQUAL ${CMAKE_SYSTEM_NAME}))
    # Add scheduler function existence info (OSX compatibility)
    add_definitions(-DHAVE_SCHEDULER=${HAVE_SCHEDULER})
endif()

if(HAVE_ASYNC)
    add_definitions(-DHAVE_ASYNC=1)
else()
    add_definitions(-DHAVE_ASYNC=0)
endif()

if(HAVE_CPP_STD_COMPLEX)
    add_definitions(-DHAVE_CPP_STD_COMPLEX=1)
endif()

if(DemoMode)
    add_definitions(-DDEMO_VERSION=1)
endif()


# Give a good guess on the best Input/Output default backends
if (JackEnable)
    SET (DefaultOutput jack CACHE STRING
        "Default Output module: [null, alsa, oss, jack, portaudio]")
    # Override with perhaps more helpful midi backends
    if (AlsaEnable)
        SET (DefaultInput alsa CACHE STRING
            "Default Input module: [null, alsa, oss, jack]")
    elseif (OssEnable)
        SET (DefaultInput oss CACHE STRING
            "Default Input module: [null, alsa, oss, jack]")
    else ()
        SET (DefaultInput jack CACHE STRING
            "Default Input module: [null, alsa, oss, jack]")
    endif ()
elseif (AlsaEnable)
    SET (DefaultOutput alsa CACHE STRING
        "Default Output module: [null, alsa, oss, jack, portaudio]")
    SET (DefaultInput alsa CACHE STRING
        "Default Input module: [null, alsa, oss, jack]")
elseif (OssEnable)
    SET (DefaultOutput oss CACHE STRING
        "Default Output module: [null, alsa, oss, jack, portaudio]")
    SET (DefaultInput oss CACHE STRING
        "Default Input module: [null, alsa, oss, jack]")
elseif (PaEnable)
    SET (DefaultOutput pa CACHE STRING
        "Default Output module: [null, alsa, oss, jack, portaudio]")
    SET (DefaultInput null CACHE STRING
        "Default Input module: [null, alsa, oss, jack]")
elseif (SndioEnable)
    SET (DefaultOutput sndio CACHE STRING
        "Default Output module: [null, alsa, oss, jack, portaudio, sndio]")
    SET (DefaultInput sndio CACHE STRING
        "Default Input module: [null, alsa, oss, jack, sndio]")
else()
    SET (DefaultOutput null CACHE STRING
        "Default Output module: [null, alsa, oss, jack, portaudio]")
    SET (DefaultInput null CACHE STRING
        "Default Input module: [null, alsa, oss, jack]")
endif()



if(GuiModule STREQUAL ntk AND NTK_FOUND)
	set (NtkGui TRUE)
elseif(GuiModule STREQUAL fltk AND FLTK_FOUND)
	set (FltkGui TRUE)
elseif(GuiModule  STREQUAL zest)
    set (ZestGui TRUE)
elseif(GuiModule STREQUAL off)
        add_definitions(-DDISABLE_GUI)
else  ()
        set (GuiModule off CACHE STRING "GUI module, either fltk, ntk, zest, or off")
        add_definitions(-DDISABLE_GUI)
	message(STATUS "GUI module defaulting to off")
endif()


#Build Flags
option (BuildForAMD_X86_64 "Build for AMD x86_64 system" OFF)
option (BuildForCore2_X86_64 "Build for Intel Core2 x86_64 system" OFF)
option (BuildForDebug "Include gdb debugging support" OFF)
option (ExtendedWarnings "Enable all useful warnings" OFF)
option (IncludeWhatYouUse "Check for useless includes" OFF)
mark_as_advanced(IncludeWhatYouUse)

set(CMAKE_BUILD_TYPE "Release")

set (BuildOptions_ExtendedWarnings "")
if (ExtendedWarnings)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # The following generates a list of warning exceptions
        # (after it enables all warnings).
        # This is for developers who want to make sure they really
        # see all possible warnings, with a few exceptions that don't
        # make sense in this code base.
        # You can remove those exceptions if they don't complain too
        # often about the code base. If you want to add exceptions,
        # be VERY CAREFUL, ask the project maintainers for
        # their consent.
        set (BuildOptions_ExtendedWarnings_List
        "-Wall" "-Wextra" "-Weverything" # start with really everything
        "-Wno-c++98-compat" "-Wno-c++98-compat-extra-semi" "-Wno-c++98-compat-pedantic" # C++11 is a zyn build requirement
        "-Wno-cast-align" # not relevant for our case
        "-Wno-double-promotion" # Used too often to fix
        "-Wno-old-style-cast" # forces you to use static_cast<> etc everywhere! too many errors
        "-Wno-padded" # this tells you to realign your structs to have better memory management
        "-Wno-zero-as-null-pointer-constant" # wants you to use nullptr in the whole code base
        "-Wno-implicit-int-conversion" # too often to fix
        "-Wno-extra-semi" # one semicolon more than useful - no danger to disable this
        "-Wno-vla-extension" "-Wno-vla" # rtosc currently heavily uses them, and they are supported by our CI
        "-Wno-exit-time-destructors" "-Wno-global-constructors" # used too often in rtosc
        "-Wno-gnu-zero-variadic-macro-arguments" # used too often in zyn
        "-Wno-shadow" "-Wno-shadow-field" # used too often in zyn
        "-Wno-cast-align" # used too often in zyn
        "-Wno-missing-prototypes" # used too often in zyn
        "-Wno-sign-conversion" # used too often in zyn
        "-Wno-float-equal" # can be disturbing in some cases
        "-Wno-switch-enum" # used too often in zyn
        )
        STRING(REPLACE ";" " " BuildOptions_ExtendedWarnings "${BuildOptions_ExtendedWarnings_List}")
        MESSAGE(STATUS "Using extended Warning options: ${BuildOptions_ExtendedWarnings}")
    else()
        MESSAGE(WARNING "\"ExtendedWarnings\" selected, but this is only supported for clang - ignoring")
    endif()
endif()

set (BuildOptions_x86_64AMD
    "-march=athlon64 -m64 -Wall -Wno-unused-parameter"
  CACHE STRING "X86_64 compiler options"
)

set (BuildOptions_X86_64Core2
    "-march=core2 -m64 -Wall -Wno-unused-parameter"
  CACHE STRING "X86_64 compiler options"
)

set (BuildOptions_NEON
    "-march=armv7-a -mfloat-abi=hard -mfpu=neon -mcpu=cortex-a9 -mtune=cortex-a9 -pipe -mvectorize-with-neon-quad -funsafe-loop-optimizations"
  CACHE STRING "Cortex_a9 compiler options"
)
set (BuildOptions_SSE
	"-msse -msse2 -mfpmath=sse"
  CACHE STRING "SSE compiler options"
)

set (BuildOptionsBasic
    "-std=c++11 -Wno-unused-parameter -O3 -ffast-math -fomit-frame-pointer"
    CACHE STRING "basic X86 compiler options"
)
STRING(APPEND BuildOptionsBasic " ${BuildOptions_ExtendedWarnings}")

set (BuildOptionsDebug
    "-std=c++11 -O0 -g3 -ggdb -Wall -Wno-unused-parameter -Wpointer-arith"
    CACHE STRING "Debug build flags")
STRING(APPEND BuildOptionsDebug " ${BuildOptions_ExtendedWarnings}")

if(${CMAKE_VERSION} VERSION_LESS "3.3.0")
    set(IwyuErr "disabled (cmake < 3.3.0)")
else()
    if(IncludeWhatYouUse)
        find_program(IwyuPath NAMES include-what-you-use iwyu)
        if(NOT IwyuPath)
            set(IwyuErr "package NOT found")
        endif()
    else()
        set(IwyuErr "disabled (IncludeWhatYouUse=OFF)")
    endif()
endif()

########### Settings dependent code ###########
# From here on, the setting variables have  been prepared so concentrate
# on the actual compiling.

# To avoid a conflict if PortAudio v1 is installed, the linker directory
# of portaudio must be specified first, so that xxx/lib/portaudio2 is
# searched before xxx/lib where PortAudio v1 gets installed.
if(PaEnable)
	include_directories(${PORTAUDIO_INCLUDE_DIRS})
	add_definitions(-DPORTAUDIO=1)
	list(APPEND AUDIO_LIBRARIES ${PORTAUDIO_LIBRARIES})
	list(APPEND AUDIO_LIBRARY_DIRS ${PORTAUDIO_LIBRARY_DIRS})
endif()

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

if(SndioEnable)
	list(APPEND AUDIO_LIBRARIES ${SNDIO_LIBRARY})
	list(APPEND AUDIO_LIBRARY_DIRS ${SNDIO_LIBRARY_DIRS})
	add_definitions(-DSNDIO=1)
endif(SndioEnable)

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

include_directories(${LIBLO_INCLUDE_DIRS})
add_definitions(-DUSE_NSM=1)
list(APPEND AUDIO_LIBRARIES ${LIBLO_LIBRARIES})
list(APPEND AUDIO_LIBRARY_DIRS ${LIBLO_LIBRARY_DIRS})
message(STATUS "Compiling with liblo")

# other include directories
include_directories(${ZLIB_INCLUDE_DIRS} ${MXML_INCLUDE_DIRS})
include_directories(${CMAKE_BINARY_DIR}/src) # for zyn-version.h ...

if(NOT ${X11_X11_LIB} STREQUAL "")
    get_filename_component(X11_LIBRARY_DIRS ${X11_X11_LIB} DIRECTORY)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_definitions(-DWIN32)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "CYGWIN")
    add_definitions(-D_GNU_SOURCE=1 -D_POSIX_THREAD_PRIO_INHERIT=1)
endif()

add_definitions(
	 -Wall
	 -Wextra
	 )

# macro similar to "check_cxx_compiler_flag_extra", however,
# it also checks for warnings that are only output if other warnings are active
macro (check_cxx_compiler_flag_extra _FLAG _FAILREGEX _RESULT)
    set(SAFE_CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS}")
    set(CMAKE_REQUIRED_DEFINITIONS "${_FLAG}")
    
    check_cxx_source_compiles("int main() { int x; return x; }" ${_RESULT}
        FAIL_REGEX "unrecognized|option"
    )

    set (CMAKE_REQUIRED_DEFINITIONS "${SAFE_CMAKE_REQUIRED_DEFINITIONS}")
endmacro ()

check_cxx_compiler_flag_extra("-Wall -Wextra -Wno-inconsistent-missing-override"
    "unrecognized|option"
    COMPILER_SUPPORTS_NO_INCONSISTENT_MISSING_OVERRIDE
    )
if(COMPILER_SUPPORTS_NO_INCONSISTENT_MISSING_OVERRIDE)
    add_definitions(-Wno-inconsistent-missing-override)
endif()

check_cxx_compiler_flag("-Wall -Wextra -Werror --system-header-prefix='FL/'"
    COMPILER_SUPPORTS_SYSTEM_HDR_PREFIX)
if(COMPILER_SUPPORTS_SYSTEM_HDR_PREFIX)
    add_definitions(--system-header-prefix="FL/")
endif()

if(NOT AVOID_ASM)
	message(STATUS "Compiling with x86 opcode support")
    add_definitions(-DASM_F2I_YES)
endif()

if (BuildForDebug)
	set (CMAKE_BUILD_TYPE "Debug")
	set (CMAKE_CXX_FLAGS_DEBUG ${BuildOptionsDebug})
	message (STATUS "Building for ${CMAKE_BUILD_TYPE}, flags: ${CMAKE_CXX_FLAGS_DEBUG}")
else (BuildForDebug)
	set (CMAKE_BUILD_TYPE "Release")
	
	set (CMAKE_CXX_FLAGS_RELEASE ${BuildOptionsBasic})
	
	if (BuildForAMD_X86_64)
		set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${BuildOptions_x86_64AMD}")
	endif (BuildForAMD_X86_64)
	
	if (BuildForCore2_X86_64)
			set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${BuildOptions_X86_64Core2}")
	endif (BuildForCore2_X86_64)
				
	if (SUPPORT_SSE)
		set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${BuildOptions_SSE}")
	endif (SUPPORT_SSE)
	
    if (SUPPORT_NEON AND NOT NoNeonPlease)
		set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${BuildOptions_NEON}")
	endif (SUPPORT_NEON AND NOT NoNeonPlease)
	
	message (STATUS "Building for ${CMAKE_BUILD_TYPE}, flags: ${CMAKE_CXX_FLAGS_RELEASE}")
endif (BuildForDebug)

if(NOT (${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
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

if(NTK_FOUND)
	mark_as_advanced(FORCE NTK_BASE_LIBRARY)
	mark_as_advanced(FORCE NTK_CONFIG_SCRIPT)
	mark_as_advanced(FORCE NTK_DIR)
	mark_as_advanced(FORCE FLTK_FLUID_EXECUTABLE)
	mark_as_advanced(FORCE NTK_FORMS_LIBRARY)
	mark_as_advanced(FORCE NTK_GL_LIBRARY)
	mark_as_advanced(FORCE NTK_IMAGES_LIBRARY)
	mark_as_advanced(FORCE NTK_INCLUDE_DIR)
	mark_as_advanced(FORCE NTK_MATH_LIBRARY)
endif(NTK_FOUND)

if(FltkGui)
	#UGLY WORKAROUND
	find_program (FLTK_CONFIG fltk-config)
	if (FLTK_CONFIG)
		execute_process (COMMAND ${FLTK_CONFIG} --use-images --ldflags OUTPUT_VARIABLE FLTK_LDFLAGS)
		string(STRIP ${FLTK_LDFLAGS} FLTK_LIBRARIES)
	endif()

	message(STATUS ${FLTK_LDFLAGS})


	set(GUI_LIBRARIES zynaddsubfx_gui ${FLTK_LIBRARIES} ${FLTK_LIBRARIES} ${OPENGL_LIBRARIES})

	add_definitions(-DFLTK_GUI)
	message(STATUS "Will build FLTK gui")

	include_directories(
			${FLTK_INCLUDE_DIR}
			"${CMAKE_CURRENT_SOURCE_DIR}/UI"
			"${CMAKE_CURRENT_BINARY_DIR}/UI"
			)

	add_subdirectory(UI)
endif()

if(NtkGui)

    find_program(FLTK_FLUID_EXECUTABLE ntk-fluid)

	message(STATUS ${NTK_LDFLAGS} ${NTK_IMAGES_LDFLAGS})

	set(GUI_LIBRARIES zynaddsubfx_gui ${NTK_LIBRARIES} ${NTK_IMAGES_LIBRARIES} ${OPENGL_LIBRARIES})

	if(X11_FOUND AND X11_Xpm_FOUND)
	     set(GUI_LIBRARIES ${GUI_LIBRARIES} ${X11_LIBRARIES} -lXpm)
	endif()

	add_definitions(-DNTK_GUI)

	message(STATUS "Will build NTK gui")

	include_directories(BEFORE
			${NTK_INCLUDE_DIRS}
			"${CMAKE_CURRENT_SOURCE_DIR}/UI"
			"${CMAKE_CURRENT_BINARY_DIR}/UI"
			)

	add_subdirectory(UI)
endif()

if(ZestGui)
    add_definitions(-DHAVE_DGL)
    add_definitions(-DZEST_GUI)
endif()

if(NOT FltkGui AND NOT NtkGui)
    set(NSM_WORKAROUND UI/NSM.C UI/NSM/Client.C)
    add_library(zynaddsubfx_gui_bridge STATIC UI/ConnectionDummy.cpp ${NSM_WORKAROUND})
    add_definitions(-DNO_UI=1)
endif()

########### General section ##############
# Following this should be only general compilation code, and no mention
# of module-specific variables

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/zyn-version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/zyn-version.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/zyn-config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/zyn-config.h)

link_directories(${AUDIO_LIBRARY_DIRS} ${ZLIB_LIBRARY_DIRS} ${FFTW_LIBRARY_DIRS} ${MXML_LIBRARY_DIRS} ${FLTK_LIBRARY_DIRS} ${NTK_LIBRARY_DIRS} ${X11_LIBRARY_DIRS})

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
add_subdirectory(Nio)
add_subdirectory(Plugin)

add_library(zynaddsubfx_core STATIC
    version.cpp
    globals.cpp
    ../tlsf/tlsf.c
    Containers/ScratchString.cpp
    Containers/NotePool.cpp
    Containers/MultiPseudoStack.cpp
	${zynaddsubfx_dsp_SRCS}
	${zynaddsubfx_effect_SRCS}
	${zynaddsubfx_misc_SRCS}
	${zynaddsubfx_params_SRCS}
	${zynaddsubfx_synth_SRCS}
	)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(PTHREAD_LIBRARY winpthread)
    set(PLATFORM_LIBRARIES ws2_32
            winmm
            wsock32
            "-static" iphlpapi
            "-static" winpthread)
elseif(APPLE)
    set(PTHREAD_LIBRARY pthread)
else()
    if(HAVE_LIBRT)
        set(PLATFORM_LIBRARIES rt)
    endif()
    set(PTHREAD_LIBRARY pthread)
endif()

target_link_libraries(zynaddsubfx_core
	${ZLIB_LIBRARIES}
	${FFTW_LIBRARIES}
	${MXML_LIBRARIES}
	${OS_LIBRARIES}
    ${PTHREAD_LIBRARY}
    rtosc rtosc-cpp)

if(IwyuErr)
    message (STATUS "Include what you use: ${IwyuErr}")
else()
    set(IwyuPathAndOptions
        ${IwyuPath}
        -Xiwyu
        --no_comments)
    set_property(TARGET zynaddsubfx_core PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                 ${IwyuPathAndOptions})
endif()

if(CompileTests)
	add_subdirectory(Tests)
endif(CompileTests)


add_executable(zynaddsubfx main.cpp)

#Warning: the required ordering of these to get proper linking depends upon the
#         phase of the moon
target_link_libraries(zynaddsubfx
    zynaddsubfx_core
	zynaddsubfx_nio
    zynaddsubfx_gui_bridge
	${GUI_LIBRARIES}
	${NIO_LIBRARIES}
	${AUDIO_LIBRARIES}
    ${PLATFORM_LIBRARIES}
    )

if (DssiEnable)
	add_library(zynaddsubfx_dssi SHARED
			UI/ConnectionDummy.cpp
			Output/DSSIaudiooutput.cpp
			globals.cpp
			Output/DSSIControl.cpp
			Output/DSSIControl.h
			Output/DSSIControlDescription.cpp
			Output/DSSIControlDescription.h)

	target_link_libraries(zynaddsubfx_dssi
		zynaddsubfx_core
		${OS_LIBRARIES}
		${LIBLO_LIBRARIES}
		${PLATFORM_LIBRARIES}
		-Wl,--no-undefined
		)
    install(TARGETS zynaddsubfx_dssi LIBRARY DESTINATION ${PluginLibDir}/dssi/)
endif()

install(TARGETS zynaddsubfx
	RUNTIME DESTINATION bin
	)
if(NtkGui)
    install(DIRECTORY ../pixmaps DESTINATION share/zynaddsubfx)
endif(NtkGui)

include(CTest)


##Summarize The Full Configuration
message(STATUS)
message(STATUS "${ColorBold}ZynAddSubFX Build Configuration")
message(STATUS             "===============================${ColorReset}")
message(STATUS)
message(STATUS "Building on a '${CMAKE_SYSTEM_NAME}' System")


macro(package_status foundvar pkg state)#optional color
    set (extra_macro_args ${ARGN})
    list(LENGTH extra_macro_args num_extra_args)
    if (${num_extra_args} GREATER 0)
        list(GET extra_macro_args 0 color)
    endif ()
    if(${foundvar})
        message(STATUS "${pkg} -- ${Green}${state}${ColorReset}")
    else()
        message(STATUS "${pkg} -- ${color}NOT ${state}${ColorReset}")
    endif()
endmacro()

package_status(PKG_CONFIG_FOUND "PkgConfig" "found"   ${Red})
package_status(ZLIB_FOUND       "zlib     " "found"   ${Red})
package_status(MXML_FOUND       "mxml     " "found"   ${Red})
package_status(FFTW_FOUND       "fftw3    " "found"   ${Red})
package_status(LIBLO_FOUND      "liblo    " "found"   ${Red})
package_status(X11_FOUND        "x11      " "found"   ${Yellow})
package_status(X11_Xpm_FOUND    "xpm      " "found"   ${Yellow})
package_status(FLTK_FOUND       "fltk     " "found"   ${Yellow})
package_status(NTK_FOUND        "ntk      " "found"   ${Yellow})
package_status(OSS_FOUND        "OSS      " "found"   ${Yellow})
package_status(ALSA_FOUND       "ALSA     " "found"   ${Yellow})
package_status(JACK_FOUND       "JACK     " "found"   ${Yellow})
package_status(PORTAUDIO_FOUND  "PA       " "found"   ${Yellow})
package_status(SNDIO_FOUND      "SNDIO    " "found"   ${Yellow})
package_status(LASH_FOUND       "Lash     " "found"   ${Yellow})
package_status(DSSI_FOUND       "DSSI     " "found"   ${Yellow})
package_status(CXXTEST_FOUND    "CxxTest  " "found"   ${Yellow})
package_status(LashEnable       "Lash     " "enabled" ${Yellow})
package_status(DssiEnable       "DSSI     " "enabled" ${Yellow})
package_status(CompileTests     "tests    " "enabled" ${Yellow})
package_status(AlsaEnable       "ALSA     " "enabled" ${Yellow})
package_status(JackEnable       "JACK     " "enabled" ${Yellow})
package_status(OssEnable        "OSS      " "enabled" ${Yellow})
package_status(PaEnable         "PA       " "enabled" ${Yellow})
package_status(SndioEnable      "SNDIO    " "enabled" ${Yellow})
#TODO GUI MODULE
package_status(HAVE_ASYNC       "c++ async" "usable"  ${Yellow})


message(STATUS "Link libraries: ${ZLIB_LIBRARY} ${FFTW_LIBRARY} ${MXML_LIBRARIES} ${AUDIO_LIBRARIES} ${OS_LIBRARIES}")
