# Sndio check, based on libkmid/configure.in.in.
# It defines ...
# It offers the following macros:
#  SNDIO_CONFIGURE_FILE(config_header) - generate a config.h, typical usage: 
#                                       SNDIO_CONFIGURE_FILE(${CMAKE_BINARY_DIR}/config-sndio.h)

# Copyright (c) 2020, Kinichiro Inoguchi
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)

# Already done by toplevel
find_library(SNDIO_LIBRARY sndio)
set(SNDIO_LIBRARY_DIR "")
if(SNDIO_LIBRARY)
   get_filename_component(SNDIO_LIBRARY_DIR ${SNDIO_LIBRARY} PATH)
endif(SNDIO_LIBRARY)

check_library_exists(sndio sio_open "${SNDIO_LIBRARY_DIR}" HAVE_LIBSNDIO)
if(HAVE_LIBSNDIO)
    message(STATUS "Found SNDIO: ${SNDIO_LIBRARY}")
else(HAVE_LIBSNDIO)
    message(STATUS "SNDIO not found")
endif(HAVE_LIBSNDIO)
set(SNDIO_FOUND ${HAVE_LIBSNDIO})

find_path(SNDIO_INCLUDES sndio.h)

get_filename_component(_FIND_SNDIO_MODULE_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)
macro(SNDIO_CONFIGURE_FILE _destFile)
    configure_file(${_FIND_SNDIO_MODULE_DIR}/config-sndio.h.cmake ${_destFile})
endmacro(SNDIO_CONFIGURE_FILE _destFile)

mark_as_advanced(SNDIO_INCLUDES SNDIO_LIBRARY)
