
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/DPF/distrho .)

add_library(ZynPhaser_lv2 SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp Phaser.cpp)
add_library(ZynPhaser_vst SHARED ${CMAKE_SOURCE_DIR}/DPF/distrho/DistrhoPluginMain.cpp Phaser.cpp)

set_target_properties(ZynPhaser_lv2 PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_LV2")
set_target_properties(ZynPhaser_lv2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY "lv2")
set_target_properties(ZynPhaser_lv2 PROPERTIES OUTPUT_NAME "ZynPhaser")
set_target_properties(ZynPhaser_lv2 PROPERTIES PREFIX "")

set_target_properties(ZynPhaser_vst PROPERTIES COMPILE_DEFINITIONS "DISTRHO_PLUGIN_TARGET_VST")
set_target_properties(ZynPhaser_vst PROPERTIES LIBRARY_OUTPUT_DIRECTORY "vst")
set_target_properties(ZynPhaser_vst PROPERTIES OUTPUT_NAME "ZynPhaser")
set_target_properties(ZynPhaser_vst PROPERTIES PREFIX "")

target_link_libraries(ZynPhaser_lv2 zynaddsubfx_core ${OS_LIBRARIES})
target_link_libraries(ZynPhaser_vst zynaddsubfx_core ${OS_LIBRARIES})

install(TARGETS ZynPhaser_lv2 LIBRARY DESTINATION ${PluginLibDir}/lv2/ZynPhaser.lv2/)
install(TARGETS ZynPhaser_vst LIBRARY DESTINATION ${PluginLibDir}/vst/)

add_custom_command(TARGET ZynPhaser_lv2 POST_BUILD
    COMMAND ../../lv2-ttl-generator $<TARGET_FILE:ZynPhaser_lv2>
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lv2)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/lv2/manifest.ttl
	${CMAKE_CURRENT_BINARY_DIR}/lv2/presets.ttl
	${CMAKE_CURRENT_BINARY_DIR}/lv2/ZynPhaser.ttl
    DESTINATION ${PluginLibDir}/lv2/ZynPhaser.lv2/)
