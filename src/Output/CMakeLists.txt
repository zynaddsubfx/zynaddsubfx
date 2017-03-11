if(PKG_CONFIG_FOUND AND NOT (${CMAKE_SYSTEM_NAME} STREQUAL "Windows"))
	pkg_check_modules(LIBSPA spa)
	if (${LIBSPA_FOUND})
		message(STATUS "SPA plugin enabled")
		include_directories(${LIBSPA_INCLUDE_DIRS})
		link_directories(${LIBSPA_LIBRARY_DIRS})
		add_library(zynaddsubfx_spa SHARED
			    ../UI/ConnectionDummy.cpp
			    ../globals.cpp
			    icon.cpp
			    icon.h
			    oscoutput.cpp
			    oscoutput.h)
		target_link_libraries(zynaddsubfx_spa
				      zynaddsubfx_core
				      ${OS_LIBRARIES}
				      ${LIBLO_LIBRARIES}
				      ${LIBSPA_LIBRARIES})
		install(TARGETS zynaddsubfx_spa LIBRARY DESTINATION ${PluginLibDir}/spa/)
	endif()
endif()

