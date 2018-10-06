find_package(Doxygen)

if(DOXYGEN_FOUND)
    # configuration variables
    set(CMAKE_DOXYGEN_INPUT_LIST ${CMAKE_SOURCE_DIR}/src/)
    set(DOXYGEN_OUTPUT_DIR html)

    configure_file(Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    add_custom_command(OUTPUT ${DOXYGEN_OUTPUT_DIR}
        DEPENDS rtosc-doc zynaddsubfx ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM)
    add_custom_target(doc DEPENDS html)
endif()

add_subdirectory(bash-completion)

