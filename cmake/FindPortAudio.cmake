# - Try to find Motif (or lesstif)
# Once done this will define:
#  PORTAUDIO_FOUND        - system has PORTAUDIO
#  PORTAUDIO_INCLUDE_DIR  - incude paths to use Motif
#  PORTAUDIO_LIBRARIES    - Link these to use Motif

SET(PORTAUDIO_FOUND 0)

IF(UNIX)
  FIND_PATH(PORTAUDIO_INCLUDE_DIR
	  portaudio.h
    /usr/include
    )

  FIND_LIBRARY(PORTAUDIO_LIBRARIES
	  portaudio
    /usr/lib
    )

ENDIF(UNIX)

# handle the QUIETLY and REQUIRED arguments and set PORTAUDIO_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PortAudio DEFAULT_MSG PORTAUDIO_LIBRARIES PORTAUDIO_INCLUDE_DIR)


MARK_AS_ADVANCED(
  PORTAUDIO_INCLUDE_DIR
  PORTAUDIO_LIBRARIES
)
