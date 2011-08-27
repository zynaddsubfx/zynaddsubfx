#Find mxml (mini xml library)

include(LibFindMacros)
libfind_pkg_check_modules(MXML mxml)
find_path(MXML_INCLUDE_DIR
    NAMES mxml.h
    PATHS ${MXML_INCLUDE_DIRS}
    )

find_library(MXML_LIBRARY
    NAMES mxml
    PATHS ${MXML_LIBRARY_DIRS}
    )

set(MXML_PROCESS_INCLUDES MXML_INCLUDE_DIR)
set(MXML_PROCESS_LIBS MXML_LIBRARY)
libfind_process(MXML)
