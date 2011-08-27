# - Find zlib
# Find the native ZLIB includes and library
#
#  ZLIB_INCLUDE_DIRS - where to find zlib.h, etc.
#  ZLIB_LIBRARIES    - List of libraries when using zlib.
#  ZLIB_FOUND        - True if zlib found.

#=============================================================================
# Copyright 2001-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

IF (zlib_INCLUDE_DIR)
  # Already in cache, be silent
  SET(zlib_FIND_QUIETLY TRUE)
ENDIF (zlib_INCLUDE_DIR)

FIND_PATH(zlib_INCLUDE_DIR zlib.h)

SET(zlib_NAMES z zlib zdll)
FIND_LIBRARY(zlib_LIBRARY NAMES ${zlib_NAMES} )
MARK_AS_ADVANCED( zlib_LIBRARY zlib_INCLUDE_DIR )

# Per-recommendation
SET(zlib_INCLUDE_DIRS "${zlib_INCLUDE_DIR}")
SET(zlib_LIBRARIES    "${zlib_LIBRARY}")

# handle the QUIETLY and REQUIRED arguments and set zlib_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(zlib DEFAULT_MSG zlib_LIBRARIES zlib_INCLUDE_DIRS)
