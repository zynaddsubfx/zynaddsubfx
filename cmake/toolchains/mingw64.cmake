set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# MinGW-W64 compiler
SET(CMAKE_AR           x86_64-w64-mingw32-gcc-ar CACHE FILEPATH "Archiver")
set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)
SET(CMAKE_LD           x86_64-w64-mingw32-gcc)

# Search for libraries only in MinGW root AND the prefix of compiled dependencies
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32 ${MINGW_PREFIX})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
