#Find Port Audio


include(LibFindMacros)
libfind_pkg_check_modules(PORTAUDIO portaudio-2.0>=19)

find_path(PORTAUDIO_INCLUDE_DIR
    NAMES portaudio.h
    PATHS ${PortAudio_INCLUDE_DIRS}
    )

find_library(PORTAUDIO_LIBRARY
    NAMES portaudio
    PATHS ${PortAudio_LIBRARY_DIRS}
    )

set(PORTAUDIO_PROCESS_INCLUDES PORTAUDIO_INCLUDE_DIR)
set(PORTAUDIO_PROCESS_LIBS PORTAUDIO_LIBRARY)
libfind_process(PORTAUDIO)
