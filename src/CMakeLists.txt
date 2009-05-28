FIND_PACKAGE(Qt4)
INCLUDE(${QT_USE_FILE})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

#settings. These cache variables can be changed before compile
#in CMakeCache.txt
SET (FFTW_VERSION 3 CACHE STRING "Version number of FFTW")
SET (OutputModule jack CACHE STRING "Output module, either alsa, jack or portaudio")


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


include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/NewUI
	${CMAKE_CURRENT_SOURCE_DIR}/NewUI)

add_subdirectory(Misc)
add_subdirectory(Input)
add_subdirectory(Output)
#add_subdirectory(NewUI)
add_subdirectory(Synth)
add_subdirectory(Seq)
add_subdirectory(Effects)
add_subdirectory(Params)
add_subdirectory(DSP)

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
	zynaddsubfxguilib)

