cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
project(zynaddsubfx)

enable_testing()
#Currently the only directory that uses cmake
add_subdirectory(src)

install(FILES AUTHORS.txt COPYING FAQ.txt HISTORY.txt README.txt
	DESTINATION share/doc/zynaddsubfx
	)
