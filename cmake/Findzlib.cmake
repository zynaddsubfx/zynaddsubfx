#  Try to find zlib
#  Once done this will define:
#  zlib_FOUND        - system has ZLIB
#  zlib_INCLUDE_DIR  - incude paths for zlib
#  zlib_LIBRARIES    - Link these to use zlib

SET(zlib_FOUND 0)

IF(UNIX)
  FIND_PATH(zlib_INCLUDE_DIR
      zlib.h
      /usr/include
      )
  FIND_LIBRARY(zlib_LIBRARIES
      NAMES z zlib
      PATHS/usr/lib /usr/local/lib
      )

ENDIF(UNIX)

# handle the QUIETLY and REQUIRED arguments and set ZLIB_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(zlib DEFAULT_MSG zlib_LIBRARIES zlib_INCLUDE_DIR)


MARK_AS_ADVANCED(
  zlib_INCLUDE_DIR
  zlib_LIBRARIES
)
