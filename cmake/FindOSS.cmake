# Find OSS (Open Sound System)

# OSS is not for APPLE or WINDOWS

IF(APPLE OR WIN32)
  RETURN()
ENDIF()

find_path(OSS_INCLUDE_DIR sys/soundcard.h)
set(OSS_LIBRARIES True)
mark_as_advanced(OSS_INCLUDE_DIR)

# handle the QUIETLY and REQUIRED arguments and set OSS_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OSS DEFAULT_MSG OSS_LIBRARIES OSS_INCLUDE_DIR)
