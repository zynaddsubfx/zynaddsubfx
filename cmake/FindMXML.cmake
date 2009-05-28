# - Try to find Motif (or lesstif)
# Once done this will define:
#  MXML_FOUND        - system has MXML
#  MXML_INCLUDE_DIR  - incude paths to use Motif
#  MXML_LIBRARIES    - Link these to use Motif

SET(MXML_FOUND 0)

IF(UNIX)
  FIND_PATH(MXML_INCLUDE_DIR
	  mxml.h
    /usr/include
    )

  FIND_LIBRARY(MXML_LIBRARIES
	  mxml
    /usr/lib
    )

ENDIF(UNIX)

# handle the QUIETLY and REQUIRED arguments and set MXML_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MXML DEFAULT_MSG MXML_LIBRARIES MXML_INCLUDE_DIR)


MARK_AS_ADVANCED(
  MXML_INCLUDE_DIR
  MXML_LIBRARIES
)
