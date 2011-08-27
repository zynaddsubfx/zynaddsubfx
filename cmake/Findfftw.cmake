#Find fftw (FFT algorithm library)

include(LibFindMacros)
libfind_pkg_check_modules(fftw fftw3)
find_path(fftw_INCLUDE_DIR
    NAMES fftw3.h
    PATHS ${fftw_INCLUDE_DIRS}
    )

find_library(fftw_LIBRARY
    NAMES fftw3
    PATHS ${fftw_LIBRARY_DIRS}
    )

set(fftw_PROCESS_INCLUDES fftw_INCLUDE_DIR)
set(fftw_PROCESS_LIBS fftw_LIBRARY)
libfind_process(fftw)
