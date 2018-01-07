
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/DPF/distrho .)

add_library(ZynEcho_lv2 SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp Echo.cpp)
add_library(ZynEcho_vst SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp Echo.cpp)

set_target_properties(ZynEcho_lv2 PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_LV2")
set_target_properties(ZynEcho_lv2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY "lv2")
set_target_properties(ZynEcho_lv2 PROPERTIES OUTPUT_NAME "ZynEcho")
set_target_properties(ZynEcho_lv2 PROPERTIES PREFIX "")

set_target_properties(ZynEcho_vst PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_VST")
set_target_properties(ZynEcho_vst PROPERTIES LIBRARY_OUTPUT_DIRECTORY "vst")
set_target_properties(ZynEcho_vst PROPERTIES OUTPUT_NAME "ZynEcho")
set_target_properties(ZynEcho_vst PROPERTIES PREFIX "")

if(APPLE)
    target_link_libraries(ZynEcho_lv2 zynaddsubfx_core ${OS_LIBRARIES} "-Wl,-exported_symbol,_lv2_descriptor" "-Wl,-exported_symbol,_lv2_generate_ttl")
    target_link_libraries(ZynEcho_vst zynaddsubfx_core ${OS_LIBRARIES} "-Wl,-exported_symbol,_VSTPluginMain")
else()
    target_link_libraries(ZynEcho_lv2 zynaddsubfx_core ${OS_LIBRARIES})
    target_link_libraries(ZynEcho_vst zynaddsubfx_core ${OS_LIBRARIES})
endif()

install(TARGETS ZynEcho_lv2 LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynEcho.lv2/)
install(TARGETS ZynEcho_vst LIBRARY DESTINATION ${PluginLibDir}/vst/)

add_custom_command(TARGET ZynEcho_lv2 POST_BUILD
    COMMAND ../../lv2-ttl-generator $<TARGET_FILE:ZynEcho_lv2>
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lv2)

add_dependencies(ZynEcho_lv2 lv2-ttl-generator)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/lv2/manifest.ttl
	${CMAKE_CURRENT_BINARY_DIR}/lv2/presets.ttl
	${CMAKE_CURRENT_BINARY_DIR}/lv2/ZynEcho.ttl
    DESTINATION ${PluginLibDir}/lv2/ZynEcho.lv2/)
