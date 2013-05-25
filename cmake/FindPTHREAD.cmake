# - Find PTHREAD
# Find the native pthreads includes and library
#
# PTHREAD_INCLUDES - where to find pthread.h
# PTHREAD_LIBRARIES - List of libraries when using pthread.
# PTHREAD_FOUND - True if pthread found.

if (PTHREAD_INCLUDES)
    # Already in cache, be silent
    set (PTHREAD_FIND_QUIETLY TRUE)
endif (PTHREAD_INCLUDES)

find_path (PTHREAD_INCLUDES
    PATHS "C:/Program Files/pthread/include"
    NAMES pthread.h sched.h semaphore.h)

find_library (PTHREAD_LIBRARIES
    PATHS "C:/Program Files/pthread/lib/x64"
    NAMES pthread pthreadVC2 libpthreadVC2)

# handle the QUIETLY and REQUIRED arguments and set FFTW_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (PTHREAD DEFAULT_MSG PTHREAD_LIBRARIES
    PTHREAD_INCLUDES)

mark_as_advanced (PTHREAD_LIBRARIES PTHREAD_INCLUDES)
