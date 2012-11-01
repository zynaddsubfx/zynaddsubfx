# The CMake supplied FindNTK delivers NTK_LIBRARIES as a full list of
# static and shared libs, with full paths. We really want just a list of
# the lib names. This slight perversion defines
#  NTK_FOUND       true or false
#  NTK_CONFIG      ntk-config executable
#  NTK_FLUID_EXECUTABLE    fluid executable
#  NTK_LDFLAGS   a list of libs required for linking
#

if (NTK_LDFLAGS)
  # in cache already
  set(NTK_FOUND TRUE)
else (NTK_LDFLAGS)
    find_program (NTK_CONFIG ntk-config)
    if (NTK_CONFIG)
        execute_process (COMMAND ${NTK_CONFIG} --use-images --ldflags OUTPUT_VARIABLE NTK_LDFLAGS)
        execute_process (COMMAND ${NTK_CONFIG} --use-images --includedir OUTPUT_VARIABLE NTK_INCLUDE_DIR)
    string(STRIP ${NTK_LDFLAGS} NTK_LIBS)
    string(REPLACE "-l" "" NTK_LIBS ${NTK_LIBS})
    string(REPLACE " " "; " NTK_LIBS ${NTK_LIBS})
    #list(APPEND NTK_LIBS ${NTK_LINK_LIBS})
    find_program (FLTK_FLUID_EXECUTABLE ntk-fluid)
        if (FLTK_FLUID_EXECUTABLE)
#            mark_as_advanced(NTK_CONFIG)
#            mark_as_advanced(NTK_EXECUTABLE)
#            mark_as_advanced(NTK_LIBRARIES)
            set(NTK_FOUND TRUE)
            set(NTK_WRAP_UI 1)
        endif(FLTK_FLUID_EXECUTABLE)
    endif (NTK_CONFIG)
endif (NTK_LDFLAGS)

# message("NTK_LDFLAGS: ${NTK_LDFLAGS}")
# message("NTK_WRAP_UI: ${NTK_WRAP_UI}")

if (NTK_FOUND)
    if (NOT NTK_FIND_QUIETLY)
        message(STATUS "found ${NTK_CONFIG}")
        # message(STATUS "found ${NTK_FLUID_EXECUTABLE}")
    endif (NOT NTK_FIND_QUIETLY)
else (NTK_FOUND)
    if (NTK_FIND_REQUIRED)
      message(FATAL_ERROR "could not find NTK, aborting.")
    endif (NTK_FIND_REQUIRED)
endif (NTK_FOUND)
