#Find Port Audio


include(LibFindMacros)
libfind_pkg_check_modules(PortAudio portaudio-2.0>=19)

find_path(PortAudio_INCLUDE_DIR
    NAMES portaudio.h
    PATHS ${PortAudio_INCLUDE_DIRS}
    )

find_library(PortAudio_LIBRARY
    NAMES portaudio
    PATHS ${Portaudio_LIBRARY_DIRS}
    )

set(PortAudio_PROCESS_INCLUDES PortAudio_INCLUDE_DIR)
set(PortAudio_PROCESS_LIBS PortAudio_LIBRARY)
libfind_process(PortAudio)
