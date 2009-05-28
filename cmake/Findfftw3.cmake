# - Try to find Motif (or lesstif)
# Once done this will define:
#  fftw3_FOUND        - system has fftw3
#  fftw3_INCLUDE_DIR  - incude paths to use Motif
#  fftw3_LIBRARIES    - Link these to use Motif

SET(fftw3_FOUND 0)

IF(UNIX)
  FIND_PATH(fftw3_INCLUDE_DIR
	  fftw3.h
    /usr/include
    )

  FIND_LIBRARY(fftw3_LIBRARIES
	  fftw3
    /usr/lib
    )

ENDIF(UNIX)

# handle the QUIETLY and REQUIRED arguments and set fftw3_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(fftw3 DEFAULT_MSG fftw3_LIBRARIES fftw3_INCLUDE_DIR)


MARK_AS_ADVANCED(
  fftw3_INCLUDE_DIR
  fftw3_LIBRARIES
)
