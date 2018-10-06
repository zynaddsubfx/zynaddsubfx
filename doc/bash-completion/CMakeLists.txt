INCLUDE(BashCompletion)
IF(COMMAND BASHCOMP_INSTALL)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/zynaddsubfx.in
		${CMAKE_CURRENT_BINARY_DIR}/zynaddsubfx @ONLY)
	BASHCOMP_INSTALL(zynaddsubfx)
ENDIF()
