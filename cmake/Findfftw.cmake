# - Try to find fftw
# Once done this will define:
#  fftw_FOUND        - system has fftw
#  fftw_INCLUDE_DIR  - incude paths to use fftw
#  fftw_LIBRARIES    - Link these to use fftw
#  fftw_VERSION      - Version of fftw found

SET(fftw_FOUND 0)

IF(UNIX OR CYGWIN)
  FIND_PATH(fftw_INCLUDE_DIR
	  fftw3.h
    /usr/include
    )

  FIND_LIBRARY(fftw_LIBRARIES
	  fftw3
    /usr/lib
    )

ENDIF(UNIX OR CYGWIN)

# handle the QUIETLY and REQUIRED arguments and set fftw_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(fftw DEFAULT_MSG fftw_LIBRARIES fftw_INCLUDE_DIR)

set(fftw_VERSION 3) #only one option for now [TODO]:extend for version two

MARK_AS_ADVANCED(
  fftw_INCLUDE_DIR
  fftw_LIBRARIES
)
