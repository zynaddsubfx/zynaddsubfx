cmake_minimum_required(VERSION 3.12)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
project(ZynAddSubFX
    DESCRIPTION "ZynAddSubFX software synthesizer"
    VERSION 3.0.7
    HOMEPAGE_URL "https://zynaddsubfx.sourceforge.io/")
string(TOLOWER "${PROJECT_NAME}" PROJECT_NAME_LCASE)
cmake_policy(SET CMP0072 NEW)

#Set data directory, if any
if(DEFINED ZYN_DATADIR)
add_definitions(-DZYN_DATADIR="${ZYN_DATADIR}")
endif()

option(ZYN_SYSTEM_RTOSC "Use system provided librtosc and librtosc-cpp" OFF)

#Include RTOSC
if(ZYN_SYSTEM_RTOSC)
    include(FindPkgConfig)
    pkg_check_modules(RTOSC REQUIRED librtosc)
    pkg_check_modules(RTOSC_CPP REQUIRED librtosc-cpp)
    include_directories(${RTOSC_INCLUDE_DIR})
    message(STATUS "Found system provided librtosc and librtosc-cpp...")
else()
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rtosc/CMakeLists.txt")
        message(STATUS "RTOSC NOT FOUND")
        message(STATUS "Attempting to checkout submodule")
        find_package(Git REQUIRED)
        execute_process(COMMAND git submodule update --init --recursive)
        if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/rtosc/CMakeLists.txt")
            message(FATAL_ERROR "FAILED TO CHECKOUT RTOSC\n"
            "please check file permissions and your network")
        endif()
    else()
        message(STATUS "Found Rtosc Submodule...")
    endif()

    set(RTOSC_NO_INSTALL TRUE)
    include("rtosc/cmake/ColorMessage.cmake")
    add_subdirectory(rtosc)
    include_directories(rtosc/include)
endif()

enable_testing()
include(CTestConfig.cmake)
#Currently the only directories that use cmake
add_subdirectory(src)
add_subdirectory(doc) # Doxygen only

install(FILES AUTHORS.txt COPYING README.adoc
	DESTINATION share/doc/zynaddsubfx
	)
install(FILES zynaddsubfx-jack-multi.desktop zynaddsubfx-jack.desktop zynaddsubfx-alsa.desktop zynaddsubfx-oss.desktop
    DESTINATION share/applications)
install(FILES zynaddsubfx.svg
    DESTINATION share/pixmaps)
install(FILES zynaddsubfx.png
    DESTINATION share/pixmaps)
install(DIRECTORY instruments/banks
    DESTINATION share/zynaddsubfx)
if(DEFINED ZYN_EXAMPLESDIR)
install(DIRECTORY instruments/examples
    DESTINATION ${ZYN_EXAMPLESDIR})
else()
install(DIRECTORY instruments/examples
    DESTINATION share/zynaddsubfx)
endif()
install(DIRECTORY instruments/ZynAddSubFX.lv2presets
    DESTINATION ${PluginLibDir}/lv2)

## uninstall target
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
               "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
               IMMEDIATE @ONLY)
add_custom_target(uninstall
                  "${CMAKE_COMMAND}" -P
                  "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake")

## CPack
SET(CPACK_PACKAGE_VENDOR             "ZynAddSubFX Developers")
SET(CPACK_RESOURCE_FILE_README       "${CMAKE_SOURCE_DIR}/README.adoc")
SET(CPACK_PACKAGE_DESCRIPTION_FILE   "${CPACK_RESOURCE_FILE_README}")
SET(CPACK_RESOURCE_FILE_LICENSE      "${CMAKE_SOURCE_DIR}/COPYING")
SET(CPACK_PACKAGE_EXECUTABLES        "${CMAKE_PROJECT_NAME_LCASE}" "${CMAKE_PROJECT_NAME}")
SET(CPACK_SOURCE_GENERATOR           "TBZ2")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME   "${CMAKE_PROJECT_NAME_LCASE}-${PROJECT_VERSION}")
set(CPACK_SOURCE_IGNORE_FILES        "/build;/\.github;/\.git/;/\.gitignore;/\.gitmodules;/\.travis.yml;/\.yamllint;/TODO-release\.md;/style\.sh;/style\.cfg;${CPACK_SOURCE_IGNORE_FILES}")
# Disable strip for Debug|RelWithDebInfo
if(CMAKE_BUILD_TYPE MATCHES "Deb")
        unset(CPACK_STRIP_FILES)
else()
        set(CPACK_STRIP_FILES TRUE)
endif()

INCLUDE(CPack)
