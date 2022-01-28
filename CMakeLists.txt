cmake_minimum_required(VERSION 3.0)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
project(zynaddsubfx)
set(VERSION_MAJOR "3")
set(VERSION_MINOR "0")
set(VERSION_REVISION "7")

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

