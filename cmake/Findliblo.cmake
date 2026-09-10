#Find Lo

find_path(LIBLO_INCLUDE_DIR
    NAMES lo/lo.h
)

find_library(LIBLO_LIBRARY
    NAMES lo
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(liblo
    REQUIRED_VARS LIBLO_LIBRARY LIBLO_INCLUDE_DIR
)

# Legacy variables
set(LIBLO_LIBRARIES ${LIBLO_LIBRARY})
set(LIBLO_INCLUDE_DIRS ${LIBLO_INCLUDE_DIR})

# Create imported target
if(liblo_FOUND AND NOT TARGET liblo::liblo)
    add_library(liblo::liblo UNKNOWN IMPORTED)

    set_target_properties(liblo::liblo PROPERTIES
        IMPORTED_LOCATION "${LIBLO_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBLO_INCLUDE_DIR}"
    )
endif()
