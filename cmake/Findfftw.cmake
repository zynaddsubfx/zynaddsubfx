#Find fftw (FFT algorithm library)

include(LibFindMacros)
libfind_pkg_check_modules(FFTW fftw3)
find_path(FFTW_INCLUDE_DIR
    NAMES fftw3.h
    PATHS ${FFTW_INCLUDE_DIRS}
    )

find_library(FFTW_LIBRARY
    NAMES fftw3
    PATHS ${FFTW_LIBRARY_DIRS}
    )

set(FFTW_PROCESS_INCLUDES FFTW_INCLUDE_DIR)
set(FFTW_PROCESS_LIBS FFTW_LIBRARY)
libfind_process(FFTW)
